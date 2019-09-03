#!/usr/bin/env python
#
# edu.py - Education Award Token Application
#
# This program implements a web-server application that allows students to
# take tests to earn award tokens granted by teachers that can be redeemed
# for tuitiion free college units.
#
# Usage: python edu.py [<port>]
#
# This program has depends on the nexus_sdk.py SDK library where the master
# copy is in LLL-TAO/sdk/nexus_sdk.py. A shadow copy is in a peer
# directory in TAO-App/sdk/nexus_sdk.py and this application directory
# symlinks to the sdk library (via "import nexus_sdk as nexus")..
#
#------------------------------------------------------------------------------

import commands
import bottle
import sys
import time
import os
import random

try:
    sdk_dir = os.getcwd().split("/")[0:-2]
    sdk_dir = "/".join(sdk_dir) + "/sdk"
    sys.path.append(sdk_dir)
    import nexus_sdk as nexus
except:
    print "Need to place nexus_sdk.py in this directory"
    exit(0)
#endtry

#------------------------------------------------------------------------------

#
# Did command line have a port number to listen to?
#
edu_port = int(sys.argv[1]) if (len(sys.argv) == 2) else 4444

#
# Indexed by nexus session-id and returns class from nexus_sdk.sdk_init().
# Allows session continuity across web pages.
#
contexts = {}

#
# API handle for account 'admin'. Created in init().
#
admin_account = None

#
# Need some delays so block verification can happen in the daemon.
#
def sleep(s=2):
    time.sleep(s)
#enddef    

#------------------------------------------------------------------------------

#
# ---------- Simple function definitions. ----------
#

def green(string):
    output = '<font color="green">{}</font>'.format(string)
    return(output)
#enddef

def red(string):
    output = '<font color="red">{}</font>'.format(string)
    return(output)
#enddef

def blue(string):
    output = '<font color="blue">{}</font>'.format(string)
    return(output)
#enddef

def cour(string):
    output = '<font face="Courier New">{}</font>'.format(string)
    return(output)
#enddef             

def bold(string):                                                              
    string = string.replace("[1m", "<b>")                                      
    string = string.replace("[0m", "</b>")                                     
    return(string)                                                             
#enddef

#------------------------------------------------------------------------------

#
# Wrapper to make all web pages look alike, from a format perspective.
#
edu_page = '''
    <html>
    <title>Nexus Education Award Token Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
         border-radius:20px;border:5px solid #666666;">
    <font face="verdana"><center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Education Award Token Application</b></a></head><br><br><hr>

    {}

    <hr></center></font></body></html>
'''
edu_page_logged_in = '''
    <html>
    <title>Nexus Education Award Token Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
         border-radius:20px;border:5px solid #666666;">
    <font face="verdana"><center>
    <br><head><a href="/logged-in/{}" style="text-decoration:none;"><font 
                 color="black">
    <b>Nexus Education Award Token Application</b></a></head><br><br><hr>

    {}

    <hr></center></font></body></html>
'''

#
# table_header/table_header1
#
# Print a consistent table header row for all displays.
#
def table_header(*args):
    output = '<center><table align="center" border="0"><tr>'

    for arg in args:
        output += '''
            <td align="center"><font face="Sans-Serif"><b>{}</b></font></td>
        '''.format(arg)
    #endfor
    output += "</tr></center>"
    return(output)
#enddef
def table_header1(*args):
    output = '<center><table align="center" border="1"><tr>'

    for arg in args:
        output += '''
            <td align="center"><font face="Sans-Serif"><b>{}</b></font></td>
        '''.format(arg)
    #endfor
    output += "</tr></center>"
    return(output)
#enddef

#
# table_row
#
# Print a consistent table row for all displays.
#
def table_row(*args):
    output = "<tr>"
    for arg in args:
        output += '''
            <td valign="top"><font face="Sans-Serif">{}</font></td>
        '''.format(arg)
    #endfor
    output += "</tr>"
    return(output)
#enddef

#
# table_row_center
#
# Print a consistent table row for all displays.
#
def table_row_center(*args):
    output = "<tr>"
    for arg in args:
        output += '''
        <td align="center" valign="top"><font face="Sans-Serif">{}</font></td>
        '''.format(arg)
    #endfor
    output += "</tr>"
    return(output)
#enddef

#
# table_footer
#
# Print a consistent table footer for all displays ending a html table.
#
def table_footer():
    return("</table>")
#enddef

#
# images
#
# For displaying images stored in png files.
#
@bottle.route('/images/<filename>', method="get")
def images_filename(filename):
    return(bottle.static_file(filename, root="./images"))
#enddef

#
# show_html
#
# Returns a ascii string to the web user. Wrapped in edu_page_logged_in.
#
def show_html(msg, sid=None):
    msg = bold(msg)
    if (sid == None): return(edu_page.format(msg))
    return(edu_page_logged_in.format(sid, msg))
#enddef

landing_page_html = '''
    <center>
    <br><form action="/login" method="post">
    {}<br><br>
    <table align="center" border="0">
    <tr>
      <td><i>Username:</i></td>
      <td><input type="text" name="username" size="40" /></td>
    </tr>
    <tr>
      <td><i>Password:</i></td>
      <td><input type="text" name="password" size="40" /></td>
    </tr>
    <tr>
      <td><i>PIN:</i></td>
      <td><input type="text" name="pin" size="40" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="{}" name="action" 
        style="background-color:transparent;border-radius:10px;height:30px;" />
    <br><br>
    <input type="submit" value="{}" name="action"
       style="background-color:transparent;border-radius:10px;height:23px;" />
    </form>
    </center>
'''

#
# Html for images used in application.
#
admin_html = '''
  <img align="center" src="/images/admin.png">
'''
student_html = '''
  <img align="center" src="/images/student.png">
'''
teacher_html = '''
  <img align="center" src="/images/teacher.png">
'''
college_html = '''
  <img align="center" src="/images/college.png">
'''

user_type_to_html = { "student" : student_html, "teacher" : teacher_html,
    "college" : college_html, "admin" : admin_html }

#
# landing_page
#
# Output landing page to web browser client.
#
@bottle.route('/')
def landing_page():
    ws = "&nbsp;&nbsp;&nbsp;&nbsp;"

    output = table_header(student_html, ws, teacher_html, ws, college_html)

    s = landing_page_html.format("<b>Students</b> Login Here",
        "Student Login", "Create Student Account")
    t = landing_page_html.format("<b>Teachers</b> Login Here",
        "Teacher Login", "Create Teacher Account")
    c = landing_page_html.format("<b>Universities</b> Login Here",
        "University Login", "Create University Account")

    output += table_row(s, ws, t, ws, c)
    output += table_footer()

    return(edu_page.format(output))
#enddef

#
# get_session_id
#
# Check to see if the session-id is cached for a user. A 2-tuple (api, output)
# is returned. One or the other will be None to check for success or failure.
#
def get_session_id(sid):
    global contexts

    if (contexts.has_key(sid) == False):
        msg = "Login session {} does not exist".format(sid)
        return(None, show_html(msg))
    #endif
    api = contexts[sid]
    return(api, "")
#enddef

#
# cache_user
#
# Save username to token account address to disk.
#
def cache_user(user, address):
    directory = "./users/"
    filename = directory + user + "-" + address
    os.system("mkdir -p {}".format(directory))
    os.system("touch {}".format(filename))
#enddef

#
# get_user
#
# Look up username in users/ directory to determine its token account address.
# 
def get_user(username):
    directory = "./users/"

    if (os.path.exists(directory)):
        cmd = "ls -1 {} | egrep {}-".format(directory, username)
        user = commands.getoutput(cmd)
        if (len(user) != 0):
            address = user.split("-")[-1]
            return(address)
        #endif
    #endif
    return(None)
#enddef

#
# login
#
# Login or Create account.
#
@bottle.post('/login')
def login():
    global contexts

    user = bottle.request.forms.get("username")
    pw = bottle.request.forms.get("password")
    pin = bottle.request.forms.get("pin")
    action = bottle.request.forms.get("action")
    create_account = (action.find("Create") != -1)

    if (user == ""):
        msg = "{} failed for empty username".format(action)
        return(show_html(msg))
    #endif

    if (user == "admin"):
        user_prefix = "admin-"
    else:
        if (action.find("Student") != -1): user_prefix = "student-"
        if (action.find("Teacher") != -1): user_prefix = "teacher-"
        if (action.find("University") != -1): user_prefix = "college-"
    #endif

    api = nexus.sdk_init(user_prefix + user, pw, pin)

    #
    # When we create a user account, create a token account so user can send
    # and receive tokens.
    #
    if (create_account):
        status = api.nexus_users_create_user()
        if (status.has_key("error")):
            r = status["error"]["message"]
            msg = "Account create failed for user '{}', {}".format(user, r)
            return(show_html(msg))
        #endif
        sleep()
    #endif

    #
    # Log into new account or existing account.
    #
    status = api.nexus_users_login_user()
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "Login failed for user '{}', {}".format(user, r)
        return(show_html(msg))
    #endif
    sid = status["result"]["session"]
    sleep()

    #
    # If new account, create award token account and scholarship token account.
    #
    if (create_account):
        ta = api.username + "-atoken"
        tn = "admin-admin:award-token"
        status = api.nexus_tokens_create_account(ta, tn)
        if (status.has_key("error")):
            r = status["error"]["message"]
            msg = "Token Account create failed for user '{}', {}".format(user,
                r)
            return(show_html(msg))
        #endif
        cache_user(ta, status["result"]["address"])
        sleep(3)

        ta = api.username + "-stoken"
        tn = "admin-admin:scholarship-token"
        status = api.nexus_tokens_create_account(ta, tn)
        if (status.has_key("error")):
            r = status["error"]["message"]
            msg = "Token Account create failed for {}', {}".format(ta, r)
            return(show_html(msg))
        #endif
        cache_user(ta, status["result"]["address"])
        sleep()

        #
        # If this is a university account, cache some scholarship data. And
        # move some scholarship-tokens to its token account
        #
        if (api.username.find("college-") != -1):
            create_scholarship(user)
            move_scholarship_tokens(api)
        #endif
    #endif

    contexts[sid] = api
    output = display_user(sid, api)
    return(output)
#enddef

#
# show_students
#
# Display student progress on module tests.
#
def show_students(sid):
    ws = "&nbsp;&nbsp;"
    students = commands.getoutput("ls -1 ./users").split("\n")
    scores = [] if (os.path.exists("./scores") == False) else \
        commands.getoutput("ls -1 ./scores").split("\n")

    output = "<hr>"
    output += "Student Class Module Progess<br><br>"
    output += table_header("Student", ws, "Module", ws, "Token Award", ws,
       "Passed/Failed", ws, "Tokens Granted", ws, "")
    output += table_row(ws)

    for score in scores:
        user, module, tok, sc = score.split("-")
        sc = float(sc)
        address = None
        for student in students:
            if (student.find(user) == -1): continue
            address = student.split("-")[-1]
            break
        #endfor
        if (address == None): continue

        pf = "?" if sc == None else green("pass") if sc >= 70 else red("fail")
        if (pf != "?"): pf += "&nbsp;&nbsp;({}%)".format(sc)
        g = get_student_granted(user, module)
        if (g or sc < 70):
            grant = ""
            g = tok if sc >= 70 else "none"
        else:
            grant = ('<a href="/grant-tokens/{}/{}/{}/{}/{}">grant ' + \
                'tokens</a>').format(user, address, module, tok, sid)
            g = red("none")
        #endif
        output += table_row_center(address, ws, module, ws, tok, ws, pf, ws,
            g, ws, grant)
    #endfor

    output += table_footer()
    return(output)
#enddef

@bottle.route('/grant-tokens/<user>/<address>/<module>/<tokens>/<sid>',
              method="get")
def grant_tokens(user, address, module, tokens, sid):
    global admin_account

    admin_address = get_token_address("admin-admin-atoken")
    api = admin_account

    status = api.nexus_tokens_debit_token_by_address(admin_address, address,
        tokens)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "debit failed for student '{}', {}".format(address, r)
        return(show_html(msg, sid))
    #endif
    txid = status["result"]["txid"]

    cache_student_granted(user, module, txid, "no")
    
    output = "Sent {} Award Tokens to {}<br>txid {}".format(tokens, address,
        txid)
    return(edu_page_logged_in.format(sid, output))
#enddef

#
# students_token_claimed
#
# Check directory ./granted to see if tokens have been claimed. If not,
# claim them and mark with filename append with "-yes".
#
def student_tokens_claimed(api, module, tokens):
    user_type = api.username.split("-")[0]
    user = api.username.split("-")[-1]
    txid, yesno = get_student_granted_txid_yesno(user, module)

    #
    # If txid aleady claimed, then return True.
    # 
    if (yesno == "yes"): return(True)

    #
    # Get token-id address from admin entry in ./users. And get this user's
    # token account address from ./users.
    #
    ta = api.username + "-atoken"
    status = api.nexus_tokens_credit_account_by_name(ta, tokens, txid)
    if (status.has_key("error")): return(False)

    #
    # Flag that tokens have been claimed.
    #
    filename = "-".join([user, module, txid, "no"])
    os.system("rm -f ./granted/{}".format(filename))
    filename = "-".join([user, module, txid, "yes"])
    os.system("touch ./granted/{}".format(filename))
    return(True)
#enddef

#
# show_classes
#
# Display available classes.
#
def show_classes(api, sid):
    ws = "&nbsp;&nbsp;"

    output = "<hr>"
    output += "Class Module Availability<br><br>"
    output += table_header("Module", ws, "Title", ws, "Token Award", ws,
        "Tests", ws, "Passed/Failed", ws, "Tokens Granted", ws,
        "Tokens Claimed")
    output += table_row(ws)
    
    classes = [ ("module1", "Algebra", 100),
                ("module2", "Geometry", 100),
                ("module3", "Calculus", 100),
                ("module4", "Linear Equations", 200),
                ("module5", "Differential Equations", 200),
                ("module6", "Computer Science", 300),
                ("module7", "Drones", 500),
                ("module8", "Artificial Intelligence", 500),
                ("module9", "IoT Everywhere", 500),
                ("module10", "LISP Overlays", 1000) ]

    user_type, user = api.username.split("-")
    for m, t, tok in classes:
        g = get_student_granted(user, m)
        if (g == False):
            c = False
            test = '<a href="/take-test/{}/{}/{}/{}">take test</a>'.format( \
                user, m, tok, sid)
        else:
            c = student_tokens_claimed(api, m, tok)
            test = "complete"
        #endif

        sc = get_student_score(user, m)
        pf = "?" if sc == None else green("pass") if sc >= 70 else red("fail")
        if (pf != "?"): pf += "&nbsp;&nbsp;({}%)".format(sc)
        g = "yes" if g == True else "no"
        c = "yes" if c == True else "no"

        output += table_row_center(m, ws, t, ws, tok, ws, test, ws, pf, ws, g,
            ws, c)
    #endfor

    output += table_footer()
    return(output)
#enddef

#
# get_student_score
#
# Return score for student's test on module.
#
def get_student_score(user, module):
    directory = "./scores/"
    if (os.path.exists(directory)):
        cmd = "ls -1 {} | egrep {}-".format(directory, user + "-" + module)
        user = commands.getoutput(cmd)
        if (len(user) != 0):
            user, module, tokens, score =  user.split("-")
            return(float(score))
        #endif
    #endfor
    return(None)
#enddef

#
# get_token_address
#
# Get user's (long form with colons)  token-id address from ./users directory.
#
def get_token_address(long_user):
    entry = commands.getoutput("ls -1 ./users/ | egrep {}-".format(long_user))
    if (entry != ""):
        address = entry.split("-")[-1]
        address = address.replace("\n", "")
        return(address)
    #endif
    return(None)
#enddef

#
# cache_student_score
#
# Record user score for module.
#
def cache_student_score(user, module, tokens, score):
    directory = "./scores/"
    filename = directory + user + "-" + module + "-" + tokens + "-"

    exists = get_student_score(user, module)
    if (exists != None):
        f = filename + str(exists)
        os.system("rm -f {}".format(f))
    #endif

    os.system("mkdir -p {}".format(directory))
    filename += str(score)
    os.system("touch {}".format(filename))
#enddef

#
# get_student_granted
#
# If file exists, then tokens granted by teacher.
#
def get_student_granted(user, module):
    directory = "./granted/"
    if (os.path.exists(directory)):
        cmd = "ls -1 {} | egrep {}".format(directory, user + "-" + module)
        user = commands.getoutput(cmd)
        if (len(user) != 0): return(True)
    #endfor
    return(False)
#enddef

#
# get_student_granted_txid_yesno
#
# If file exists, then return the txid and appended "yes" or "no".
#
def get_student_granted_txid_yesno(user, module):
    directory = "./granted/"
    if (os.path.exists(directory)):
        cmd = "ls -1 {} | egrep {}-".format(directory, user + "-" + module)
        user = commands.getoutput(cmd)
        if (len(user) != 0):
            u, m, txid, yesno = user.split("-")
            yesno = yesno.replace("\n", "")
            return([txid, yesno])
        #endif
    #endfor
    return(None, None)
#enddef

#
# cache_student_granted
#
# Record user score for module.
#
def cache_student_granted(user, module, txid, yesno):
    directory = "./granted/"
    os.system("mkdir -p {}".format(directory))
    filename = directory + user + "-" + module + "-" + txid + "-" + yesno
    os.system("touch {}".format(filename))
#enddef

@bottle.route('/take-test/<user>/<module>/<tokens>/<sid>', method="get")
def take_test(user, module, tokens, sid):
    score = float(random.randint(500, 1000)) / 10
    cache_student_score(user, module, tokens, score)

    passfail = green("You Passed!") if (score >= 70) else red("You Failed!")

    output = "Your Test Score is {}%<br>{}".format(score, passfail)
    return(edu_page_logged_in.format(sid, output))
#enddef

button_refresh = '''
    <a href="/logged-in/{}">
    <button style="background-color:transparent;border-radius:10px;
        height:30px;" type="button">Refresh</button></a>
'''
button_sigchain = '''
    <a href="/account-info/{}">
    <button style="background-color:transparent;border-radius:10px;
        height:30px;" type="button">Account Info</button></a>
'''

colleges = { "asu" : ("500", "1000"), "usc" : ("1000", "10000"),
    "stanford" : ("1500", "10000"), "mit" : ("1500", "10000") }

#
# create_scholarship
#
# Create scholarship and store in ./scholarships directory.
#
def create_scholarship(user):
    directory = "./scholarships/"
    os.system("mkdir -p {}".format(directory))

    #
    # Filename is <college>-<award-token>-<usd-value>
    #
    award, usd = ("500", "500")
    if (colleges.has_key(user)): award, usd = colleges[user]
    
    filename = user + "-" + award + "-" + usd
    os.system("touch {}{}".format(directory, filename))
#enddef

#
# move_scholarship_tokens
#
# Move 10 scholarship-tokens from reserve to univeristy's token account.
#
def move_scholarship_tokens(college):
    tokens = 10
    cta = college.username + "-stoken"
    tn = "scholarship-token"
    api = admin_account

    #
    # Do debit from token-id.
    #
    dta = college.username + ":" + cta
    status = api.nexus_tokens_debit_token_by_name(tn, dta, tokens)
    if (status.has_key("error")): return(False)

    txid = status["result"]["txid"]
    sleep()
    
    #
    # Do credi to unversity token account.
    #
    status = college.nexus_tokens_credit_account_by_name(cta, tokens, txid)
    if (status.has_key("error")): return(False)

    return(True)
#enddef

#
# get_scholarsip_balance
#
# Get token account balance for scholarship-token for each university.
#
def get_scholarship_balance(user):
    global admin_account
    
    ta = "college-{}:college-{}-stoken".format(user, user)

    status = admin_account.nexus_tokens_get_account_by_name(ta)
    if (status.has_key("error")): return("?")

    bal = str(status["result"]["balance"])
    bal = bal.replace(".0", "")
    return(bal)
#enddef

#
# get_scholarship_cost
#
# Get university award token cost per scholarship token.
#
def get_scholarship_cost(user):
    cmd = "ls -1 ./scholarships | egrep {}".format(user)
    entry = commands.getoutput(cmd)
    if (len(entry) != 0):
        college, cost, usd = entry.split("-")
        return(int(cost))
    #endif
    return(None)
#enddef

buy_stoken_html = '''
    <form action="/buy-stokens/{}/{}" method="post">
    <input type="text" name="tokens" size="5" />
    <input type="submit" value="buy scholarship tokens" name="action" /> 
    </form>
'''

#
# show_scholarships
#
# Show scholarship offerings. They are created when university usernames
# are created.
#
def show_scholarships(sid):
    if (os.path.exists("./scholarships") == False): return("")

    entries = commands.getoutput("ls -1 ./scholarships").split("\n")

    ws = "&nbsp;&nbsp;"
    output = "<hr size='5'>"
    output += "University Scholarship Offerings<br><br>"

    output += table_header("University", ws, "Available<br>Scholarship Tokens",
        ws, "Price (in Award Tokens) per<br>Scholarship Token", ws,
        "Value (in USD) per<br>Scholarship Token", ws)
    output += table_row(ws)

    for entry in entries:
        user, atok, usd = entry.split("-")
        value = "${}".format(usd)
        stok = get_scholarship_balance(user)
        buy = ""
        if (stok != "?" and stok != "0"): 
            buy = "" if (sid == None) else buy_stoken_html.format(user, sid)
        #endif
        output += table_row_center(user, ws, stok, ws, atok, ws, value, ws,
            buy)
    #endfor

    output += table_footer()
    return(output)
#enddef

@bottle.route('/buy-stokens/<user>/<sid>', method="post")
def buy_stokens(user, sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)
    
    buy_stokens = int(bottle.request.forms.get("tokens"))
    cost = get_scholarship_cost(user)
    if (cost == None):
        output = "Cannot determine cost for scholarship-token at {}".format()
    else:
        output = "The cost for 1 scholarship token is {}".format(cost)
    #endif

    output = spend_atokens_for_stokens(api, user, buy_stokens, cost)

    return(edu_page_logged_in.format(sid, output))
#enddef

#
# spend_atokens_for_stokens
#
# Move 100 scholarship-tokens from reserve to univeristy's token account.
#
def spend_atokens_for_stokens(student, user, buy_stokens, cost):
    global contexts
    
    long_user = "college-" + user

    #
    # Find college token account for scholarship-token.
    #
    college = None
    for api in contexts.values():
        if (api.username != long_user): continue
        college = api
        break
    #endfor
    if (college == None):
        return(("Could not find university account '{}'<br>" + \
                "University needs to login").format(long_user))
    #endif

    total_cost = buy_stokens * cost

    #
    # Debit total_cost, in award tokens, from student and credit to college.
    #
    from_ta = student.username + "-atoken"
    to_ta = college.username + ":" + college.username + "-atoken"

    s = student.nexus_tokens_debit_account_by_name(from_ta, to_ta, total_cost)
    if (s.has_key("error")):
        m = bold(s["error"]["message"])
        return(("Debit from token account '{}' failed for {} award " + \
               "tokens<br>{}").format(from_ta, total_cost, m))
    #endif
    txid = s["result"]["txid"]
    sleep()

    to_ta = college.username + "-atoken"
    s = college.nexus_tokens_credit_account_by_name(to_ta, total_cost, txid)
    if (s.has_key("error")):
        m = bold(s["error"]["message"])
        return(("Credit to token account '{}' failed for {} award " + \
               "tokens<br>{}").format(to_ta, total_cost, m))
    #endif

    sleep(3)

    #
    # Now move scholarship tokens from college to student.
    #
    from_ta = college.username + "-stoken"
    to_ta = student.username + ":" + student.username + "-stoken"

    s = college.nexus_tokens_debit_account_by_name(from_ta, to_ta, buy_stokens)
    if (s.has_key("error")):
        m = bold(s["error"]["message"])
        return(("Debit from token account '{}' failed for {} scholarship " + \
            "tokens<br>{}").format(from_ta, buy_stokens, m))
    #endif
    txid = s["result"]["txid"]
    sleep()

    to_ta = student.username + "-stoken"
    s = student.nexus_tokens_credit_account_by_name(to_ta, buy_stokens, txid)
    if (s.has_key("error")):
        m = bold(s["error"]["message"])
        return(("Credit to token account '{}' failed for {} scholarship " + \
            "tokens<br>{}").format(to_ta, buy_stokens, m))
    #endif

    #
    # All 4 transactions succeeded!
    #
    output = ("'{}' bought {} scholarship tokens for {} award tokens " + \
        "from '{}'").format(student.username, buy_stokens, total_cost,
        college.username)
    return(output)
#enddef
    
#
# display_user
#
# Show user data and transaction history.
#
def display_user(sid, api):
    user_type, user = api.username.split("-")
    user_image = user_type_to_html[user_type]

    ws = "&nbsp;"
    tab1 = table_header(ws, ws)
    tab1 += table_row("Username:", user)
    tab1 += table_row("Genesis-ID:", api.genesis_id)
    tab1 += table_row(ws, ws)

    if (user != "admin"):
        ta = api.username + "-atoken"
        status = api.nexus_tokens_get_account_by_name(ta)
        balance = "?" if status.has_key("error") else \
            str(status["result"]["balance"])

        tab1 += table_row("Award Token Account Name:", api.username)
        address = get_user(ta)
        if (address == None): address = "?"
        tab1 += table_row("Award Token Account Address:", address)
        balance = balance.replace(".0", "")
        balance = "<b>{}</b>".format(balance)
        tab1 += table_row("Award Token-Name:", "admin:award-token")
        tab1 += table_row("Award Token Balance:", balance)
        tab1 += table_row(ws, ws)

        ta = api.username + "-stoken"
        status = api.nexus_tokens_get_account_by_name(ta)
        balance = "?" if status.has_key("error") else \
            str(status["result"]["balance"])

        tab1 += table_row("Scholarship Token Account Name:", ta)
        address = get_user(ta)
        if (address == None): address = "?"
        tab1 += table_row("Scholarship Token Account Address:", address)
        balance = balance.replace(".0", "")
        balance = "<b>{}</b>".format(balance)
        tab1 += table_row("Scholarship Token-Name:", "admin:scholarship-token")
        tab1 += table_row("Scholarship Token Balance:", balance)
    #endif
    tab1 += table_row(ws, ws)
    tab1 += table_footer()

    output = table_header(user_image, ws, ws, tab1)
    output += table_footer()

    output += button_refresh.format(sid) + "&nbsp;" + \
        button_sigchain.format(sid)

    #
    # Show classes for student display.
    #
    s = None
    if (api.username.find("student") != -1):
        output += show_classes(api, sid)
        s = sid
    #endif

    #
    # Show student progress for teacher display.
    #
    if (api.username.find("teacher") != -1):
        output += show_students(sid)
    #endif

    #
    # Show scholarship opportunities on all pages.
    #
    output += show_scholarships(s)

    return(edu_page.format(output))
#enddef

#
# logged-in
#
# Display page with genesis-ID QR-code.
#
@bottle.get('/logged-in/<sid>')
def logged_in(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    output = display_user(sid, api)
    return(output)
#enddef

form_header = '''
    <br><form action="/{}/{}" method="post">
    <table align="center" border="0">
'''
form_row = '''
    <tr>
      <td><i>{}:</i></td>
      <td><input type="text" name="{}" size="70" /></td>
    </tr>
'''
form_footer = '''
    </table>
    <br>
    <input type="submit" value="{}" 
           style="background-color:transparent;border-radius:10px;" />
    </form>
'''

#
# init
#
# Create admin account and login. Create token-id=2 to be the award token.
# Create a token account for user admin.
#
def init():
    global admin_account
    
    #
    # Need to create/use an account to create the token.
    #
    api = nexus.sdk_init("admin-admin", "password", "1234")
    admin_account = api
    login = api.nexus_users_login_user()
    if (login.has_key("error")):
        create = api.nexus_users_create_user()
        if (create.has_key("error")):
            if (api.nexus_users_login_user().has_key("error")):
                print "Account create for 'admin' failed, exit"
                exit(1)
            #endif
        #endif
        print "Created user account 'admin'"
        sleep()
        login = api.nexus_users_login_user()
    #endif
    print "User account 'admin' logged in"

    #
    # Create the award token if it does not exist.
    #
    tn = "award-token"
    supply = 1000000
    get = api.nexus_tokens_get_token_by_name(tn)
    if (get.has_key("error")):
        create = api.nexus_tokens_create_token(tn, supply, 3)
        if (create.has_key("error")):
            r = create["error"]["message"]
            print "tokens/create token {} failed, {}".format(tn, r)
            exit(1)
        #endif
        cache_user(api.username + "-atoken", create["result"]["address"])
        print "Created {} award tokens".format(supply)
        sleep()
    else:
        s = get["result"]["maxsupply"]
        c = get["result"]["currentsupply"]
        print "Token-Name '{}' exists, supply {}, current {}".format(tn, s, c)
    #endif

    #
    # Create the scholarship token if it does not exist.
    #
    tn = "scholarship-token"
    supply = 1000
    get = api.nexus_tokens_get_token_by_name(tn)
    if (get.has_key("error")):
        create = api.nexus_tokens_create_token(tn, supply, 3)
        if (create.has_key("error")):
            r = create["error"]["message"]
            print "tokens/create token {} failed, {}".format(tn, r)
            exit(1)
        #endif
        cache_user(api.username + "-stoken", create["result"]["address"])
        print "Created {} scholarship tokens".format(supply)
    else:
        s = get["result"]["maxsupply"]
        c = get["result"]["currentsupply"]
        print "Token-Name '{}' exists, supply {}, current {}".format(tn, s, c)
    #endif
#enddef

account_info_header = '''
    <table>
    <tr valign="top" align="left">
    <td align="center"><b>{}</b></td><td>&nbsp;</td><td>&nbsp;</td>
    <td><b>{}</b></td>
    </tr>
'''
account_info_row = '''
    <tr><td>{}</td>
    <td style="word-break:break-all;">{}</td>
'''

account_info_footer = "</table>"

#
# display_account_info
#
# Display account transactions or notifications with this single routing. The
# data is different so be robust enough to handle that fact.
#
def display_account_info(transactions):
    output = account_info_header.format("", "")

    for tx in transactions:
        if (type(tx) != dict):
            for t in tx:
                output += account_info_row.format(t, "")
                output += account_info_row.format("&nbsp;", "&nbsp;")
            #endif
            continue
        #endif
            
        if (tx.has_key("txid")):
            txid = tx["txid"]
            output += account_info_row.format("txid:", txid)
        #endif

        if (tx.has_key("contracts") == False or tx["contracts"] == None):
            output += account_info_row.format(tx, "")
            output += account_info_row.format("&nbsp;", "&nbsp;")
            continue
        #endif

        for contract in tx["contracts"]:
            op = contract["OP"]
            op = "OP: {}&nbsp;&nbsp;".format(op)
            address = ""
            if (contract.has_key("address")):
                address = contract["address"]
                address = "address: {}".format(address)
            #endif
            output += account_info_row.format(op, address)
        #endfor
        output += account_info_row.format("&nbsp;", "&nbsp;")
    #endfor
    output += account_info_footer

    return(output)
#enddef

#
# account_info
#
# Show sigchain transactions and notification queue.
#
@bottle.route('/account-info/<sid>', method="get")
def account_info(sid):
    api, output = get_session_id(sid)
    if (api == None): return(edu_page_logged_in.format(sid, output))

    output = ""
    
    output += "<br>"
    ts = api.nexus_users_list_transactions_by_username(0, 50)
    if (ts.has_key("error")):
        output += "Could not retrieve account transactions"
    else:
        output += "Transactions for User <b>{}</b>".format(api.username)
        output += "<br><br>"
        output += display_account_info(ts["result"])
    #endif
    output += "<hr>"

    output += "<br>"
    ns = api.nexus_users_list_notifications_by_username(0, 50)
    if (ns.has_key("error")):
        output += "Could not retrieve account notifications"
    else:
        output += "Notifications Posted for User <b>{}</b>".format( \
            api.username)
        output += "<br><br>"
        output += display_account_info(ns["result"])
    #endif
    output += "<hr>"

    return(edu_page_logged_in.format(sid, output))
#enddef

#------------------------------------------------------------------------------

#
# ---------- Main program entry point. ----------
#
date = commands.getoutput("date")
print "edu starting up at {}".format(date)

#
# Create some token reserves.
#
init()

#
# Run web server.
#
bottle.run(host="0.0.0.0", port=edu_port, debug=True)
exit(0)

#------------------------------------------------------------------------------
