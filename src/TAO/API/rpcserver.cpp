/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


namespace Net
{
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLStream;

    void ThreadRPCServer2(void* parg);

    Object JSONRPCError(int code, const string& message)
    {
        Object error;
        error.push_back(Pair("code", code));
        error.push_back(Pair("message", message));
        return error;
    }

    static const CRPCCommand vRPCCommands[] =
    { //  name                      function                 safe mode?
      //  ------------------------  -----------------------  ----------
        { "help",                   &help,                   true  }
    };

    CRPCTable::CRPCTable()
    {
        uint32_t vcidx;
        for (vcidx = 0; vcidx < (sizeof(vRPCCommands) / sizeof(vRPCCommands[0])); vcidx++)
        {
            const CRPCCommand *pcmd;

            pcmd = &vRPCCommands[vcidx];
            mapCommands[pcmd->name] = pcmd;
        }
    }

    const CRPCCommand *CRPCTable::operator[](string name) const
    {
        map<string, const CRPCCommand*>::const_iterator it = mapCommands.find(name);
        if (it == mapCommands.end())
            return NULL;
        return (*it).second;
    }

    //
    // HTTP protocol
    //
    // This ain't Apache.  We're just using HTTP header for the length field
    // and to be compatible with other JSON-RPC implementations.
    //

    std::string FormatFullVersion()
    {
        return "Tritium Alpha";
    }

    std::string HTTPPost(const std::string& strMsg, const std::map<std::string, std::string>& mapRequestHeaders)
    {
        std::ostringstream ssHeader;
        ssHeader << "POST / HTTP/1.1\r\n"
          << "User-Agent: Nexus-json-rpc/" << FormatFullVersion() << "\r\n"
          << "Host: 127.0.0.1\r\n"
          << "Content-Type: application/json\r\n"
          << "Content-Length: " << strMsg.size() << "\r\n"
          << "Connection: close\r\n"
          << "Accept: application/json\r\n";

        for(auto item : mapRequestHeaders)
            ssHeader << item.first << ": " << item.second << "\r\n";

        ssHeader << "\r\n" << strMsg;

        return ssHeader.str();
    }



    static std::string HTTPReply(int nStatus, const std::string& strMsg)
    {
        if (nStatus == 401)
            return strprintf("HTTP/1.0 401 Authorization Required\r\n"
                "Date: %s\r\n"
                "Server: Nexus-json-rpc/%s\r\n"
                "WWW-Authenticate: Basic realm=\"jsonrpc\"\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 296\r\n"
                "\r\n"
                "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\r\n"
                "\"http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd\">\r\n"
                "<HTML>\r\n"
                "<HEAD>\r\n"
                "<TITLE>Error</TITLE>\r\n"
                "<META HTTP-EQUIV='Content-Type' CONTENT='text/html; charset=ISO-8859-1'>\r\n"
                "</HEAD>\r\n"
                "<BODY><H1>401 Unauthorized.</H1></BODY>\r\n"
                "</HTML>\r\n", rfc1123Time().c_str(), FormatFullVersion().c_str());

        const char *cStatus;
             if (nStatus == 200) cStatus = "OK";
        else if (nStatus == 400) cStatus = "Bad Request";
        else if (nStatus == 403) cStatus = "Forbidden";
        else if (nStatus == 404) cStatus = "Not Found";
        else if (nStatus == 500) cStatus = "Internal Server Error";
        else cStatus = "";
        return strprintf(
                "HTTP/1.1 %d %s\r\n"
                "Date: %s\r\n"
                "Connection: close\r\n"
                "Content-Length: %d\r\n"
                "Content-Type: application/json\r\n"
                "Server: Nexus-json-rpc/%s\r\n"
                "\r\n"
                "%s",
            nStatus,
            cStatus,
            rfc1123Time().c_str(),
            strMsg.size(),
            FormatFullVersion().c_str(),
            strMsg.c_str());
    }

    int ReadHTTPStatus(std::basic_istream<char>& stream)
    {
        string str;
        getline(stream, str);
        vector<string> vWords;
        boost::split(vWords, str, boost::is_any_of(" "));
        if (vWords.size() < 2)
            return 500;
        return atoi(vWords[1].c_str());
    }

    int ReadHTTPHeader(std::basic_istream<char>& stream, map<string, string>& mapHeadersRet)
    {
        int nLen = 0;
        loop()
        {
            string str;
            std::getline(stream, str);
            if (str.empty() || str == "\r")
                break;
            string::size_type nColon = str.find(":");
            if (nColon != string::npos)
            {
                string strHeader = str.substr(0, nColon);
                boost::trim(strHeader);
                boost::to_lower(strHeader);
                string strValue = str.substr(nColon+1);
                boost::trim(strValue);
                mapHeadersRet[strHeader] = strValue;
                if (strHeader == "content-length")
                    nLen = atoi(strValue.c_str());
            }
        }
        return nLen;
    }

    int ReadHTTP(std::basic_istream<char>& stream, map<string, string>& mapHeadersRet, string& strMessageRet)
    {
        mapHeadersRet.clear();
        strMessageRet = "";

        // Read status
        int nStatus = ReadHTTPStatus(stream);

        // Read header
        int nLen = ReadHTTPHeader(stream, mapHeadersRet);
        if (nLen < 0 || nLen > (int)MAX_SIZE)
            return 500;

        // Read message
        if (nLen > 0)
        {
            vector<char> vch(nLen);
            stream.read(&vch[0], nLen);
            strMessageRet = string(vch.begin(), vch.end());
        }

        return nStatus;
    }

    bool HTTPAuthorized(map<string, string>& mapHeaders)
    {
        string strAuth = mapHeaders["authorization"];
        if (strAuth.substr(0,6) != "Basic ")
            return false;
        string strUserPass64 = strAuth.substr(6); boost::trim(strUserPass64);
        string strUserPass = DecodeBase64(strUserPass64);
        return strUserPass == strRPCUserColonPass;
    }

    //
    // JSON-RPC protocol.  Nexus speaks version 1.0 for maximum compatibility,
    // but uses JSON-RPC 1.1/2.0 standards for parts of the 1.0 standard that were
    // unspecified (HTTP errors and contents of 'error').
    //
    // 1.0 spec: http://json-rpc.org/wiki/specification
    // 1.2 spec: http://groups.google.com/group/json-rpc/web/json-rpc-over-http
    // http://www.codeproject.com/KB/recipes/JSON_Spirit.aspx
    //

    string JSONRPCRequest(const string& strMethod, const Array& params, const Value& id)
    {
        Object request;
        request.push_back(Pair("method", strMethod));
        request.push_back(Pair("params", params));
        request.push_back(Pair("id", id));
        return write_string(Value(request), false) + "\n";
    }

    string JSONRPCReply(const Value& result, const Value& error, const Value& id)
    {
        Object reply;
        if (error.type() != null_type)
            reply.push_back(Pair("result", Value::null));
        else
            reply.push_back(Pair("result", result));
        reply.push_back(Pair("error", error));
        reply.push_back(Pair("id", id));
        return write_string(Value(reply), false) + "\n";
    }

    void ErrorReply(std::ostream& stream, const Object& objError, const Value& id)
    {
        // Send error reply from json-rpc error object
        int nStatus = 500;
        int code = find_value(objError, "code").get_int();
        if (code == -32600) nStatus = 400;
        else if (code == -32601) nStatus = 404;
        string strReply = JSONRPCReply(Value::null, objError, id);
        stream << HTTPReply(nStatus, strReply) << std::flush;
    }

    bool ClientAllowed(const string& strAddress)
    {
        if (strAddress == asio::ip::address_v4::loopback().to_string())
            return true;
        const vector<string>& vAllow = mapMultiArgs["-rpcallowip"];
        BOOST_FOREACH(string strAllow, vAllow)
            if (WildcardMatch(strAddress, strAllow))
                return true;
        return false;
    }

    //
    // IOStream device that speaks SSL but can also speak non-SSL
    //
    class SSLIOStreamDevice : public iostreams::device<iostreams::bidirectional> {
    public:
        SSLIOStreamDevice(SSLStream &streamIn, bool fUseSSLIn) : stream(streamIn)
        {
            fUseSSL = fUseSSLIn;
            fNeedHandshake = fUseSSLIn;
        }

        void handshake(ssl::stream_base::handshake_type role)
        {
            if (!fNeedHandshake) return;
            fNeedHandshake = false;
            stream.handshake(role);
        }
        std::streamsize read(char* s, std::streamsize n)
        {
            handshake(ssl::stream_base::server); // HTTPS servers read first
            if (fUseSSL) return stream.read_some(asio::buffer(s, n));
            return stream.next_layer().read_some(asio::buffer(s, n));
        }
        std::streamsize write(const char* s, std::streamsize n)
        {
            handshake(ssl::stream_base::client); // HTTPS clients write first
            if (fUseSSL) return asio::write(stream, asio::buffer(s, n));
            return asio::write(stream.next_layer(), asio::buffer(s, n));
        }
        bool connect(const std::string& server, const std::string& port)
        {
            ip::tcp::resolver resolver(stream.get_io_service());
            ip::tcp::resolver::query query(server.c_str(), port.c_str());
            ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            ip::tcp::resolver::iterator end;
            boost::system::error_code error = asio::error::host_not_found;
            while (error && endpoint_iterator != end)
            {
                stream.lowest_layer().close();
                stream.lowest_layer().connect(*endpoint_iterator++, error);
            }
            if (error)
                return false;
            return true;
        }

    private:
        bool fNeedHandshake;
        bool fUseSSL;
        SSLStream& stream;
    };

    void ThreadRPCServer(void* parg)
    {
        IMPLEMENT_RANDOMIZE_STACK(ThreadRPCServer(parg));


        try
        {
            vnThreadsRunning[THREAD_RPCSERVER]++;
            ThreadRPCServer2(parg);
            vnThreadsRunning[THREAD_RPCSERVER]--;
        }
        catch (std::exception& e) {
            vnThreadsRunning[THREAD_RPCSERVER]--;
            PrintException(&e, "ThreadRPCServer()");
        } catch (...) {
            vnThreadsRunning[THREAD_RPCSERVER]--;
            PrintException(NULL, "ThreadRPCServer()");
        }

        //delete pMiningKey; pMiningKey = NULL;

        printf("ThreadRPCServer exiting\n");
    }

    void ThreadRPCServer2(void* parg)
    {
        printf("ThreadRPCServer started\n");

        strRPCUserColonPass = mapArgs["-rpcuser"] + ":" + mapArgs["-rpcpassword"];
        if (mapArgs["-rpcpassword"] == "")
        {
            uint8_t rand_pwd[32];
            RAND_bytes(rand_pwd, 32);
            string strWhatAmI = "To use Nexus";
            if (mapArgs.count("-server"))
                strWhatAmI = strprintf(_("To use the %s option"), "\"-server\"");
            else if (mapArgs.count("-daemon"))
                strWhatAmI = strprintf(_("To use the %s option"), "\"-daemon\"");
            ThreadSafeMessageBox(strprintf(
                _("%s, you must set a rpcpassword in the configuration file:\n %s\n"
                  "It is recommended you use the following random password:\n"
                  "rpcuser=rpcserver\n"
                  "rpcpassword=%s\n"
                  "(you do not need to remember this password)\n"
                  "If the file does not exist, create it with owner-readable-only file permissions.\n"),
                    strWhatAmI.c_str(),
                    GetConfigFile().string().c_str(),
                    Wallet::EncodeBase58(&rand_pwd[0],&rand_pwd[0]+32).c_str()),
                _("Error"), wxOK | wxMODAL);
            StartShutdown();
            return;
        }

        bool fUseSSL = GetBoolArg("-rpcssl");
        asio::ip::address bindAddress = mapArgs.count("-rpcallowip") ? asio::ip::address_v4::any() : asio::ip::address_v4::loopback();

        asio::io_service io_service;
        ip::tcp::endpoint endpoint(bindAddress, GetArg("-rpcport", fTestNet? TESTNET_RPC_PORT : RPC_PORT));
        ip::tcp::acceptor acceptor(io_service);
        try
        {
            acceptor.open(endpoint.protocol());
            acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen(socket_base::max_connections);
        }
        catch(boost::system::system_error &e)
        {
            ThreadSafeMessageBox(strprintf(_("An error occured while setting up the RPC port %i for listening: %s"), endpoint.port(), e.what()),
                                 _("Error"), wxOK | wxMODAL);
            StartShutdown();
            return;
        }

        ssl::context context(ssl::context::sslv23);
        if (fUseSSL)
        {
            context.set_options(ssl::context::no_sslv2);

            filesystem::path pathCertFile(GetArg("-rpcsslcertificatechainfile", "server.cert"));
            if (!pathCertFile.is_complete()) pathCertFile = filesystem::path(GetDataDir()) / pathCertFile;
            if (filesystem::exists(pathCertFile)) context.use_certificate_chain_file(pathCertFile.string());
            else printf("ThreadRPCServer ERROR: missing server certificate file %s\n", pathCertFile.string().c_str());

            filesystem::path pathPKFile(GetArg("-rpcsslprivatekeyfile", "server.pem"));
            if (!pathPKFile.is_complete()) pathPKFile = filesystem::path(GetDataDir()) / pathPKFile;
            if (filesystem::exists(pathPKFile)) context.use_private_key_file(pathPKFile.string(), ssl::context::pem);
            else printf("ThreadRPCServer ERROR: missing server private key file %s\n", pathPKFile.string().c_str());

            string strCiphers = GetArg("-rpcsslciphers", "TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH");
            SSL_CTX_set_cipher_list(context.native_handle(), strCiphers.c_str());
        }

        loop() {
            // Accept connection
            SSLStream sslStream(io_service, context);
            SSLIOStreamDevice d(sslStream, fUseSSL);
            iostreams::stream<SSLIOStreamDevice> stream(d);

            ip::tcp::endpoint peer;
            vnThreadsRunning[THREAD_RPCSERVER]--;
            acceptor.accept(sslStream.lowest_layer(), peer);
            vnThreadsRunning[4]++;
            if (fShutdown)
                return;

            // Restrict callers by IP
            if (!ClientAllowed(peer.address().to_string()))
            {
                // Only send a 403 if we're not using SSL to prevent a DoS during the SSL handshake.
                if (!fUseSSL)
                    stream << HTTPReply(403, "") << std::flush;

                printf("[RPC] Failed to Authorize New Connection.");
                continue;
            }

            map<string, string> mapHeaders;
            string strRequest;

            boost::thread api_caller(ReadHTTP, boost::ref(stream), boost::ref(mapHeaders), boost::ref(strRequest));
            if (!api_caller.timed_join(boost::posix_time::seconds(GetArg("-rpctimeout", 30))))
            {   // Timed out:
                acceptor.cancel();
                printf("ThreadRPCServer ReadHTTP timeout\n");
                continue;
            }

            // Check authorization
            if (mapHeaders.count("authorization") == 0)
            {
                stream << HTTPReply(401, "") << std::flush;
                continue;
            }
            if (!HTTPAuthorized(mapHeaders))
            {
                printf("ThreadRPCServer incorrect password attempt from %s\n",peer.address().to_string().c_str());
                /* Deter brute-forcing short passwords.
                   If this results in a DOS the user really
                   shouldn't have their RPC port exposed.*/
                if (mapArgs["-rpcpassword"].size() < 20)
                    Sleep(250);

                stream << HTTPReply(401, "") << std::flush;
                continue;
            }

            Value id = Value::null;
            try
            {
                // Parse request
                Value valRequest;
                if (!read_string(strRequest, valRequest) || valRequest.type() != obj_type)
                    throw JSONRPCError(-32700, "Parse error");
                const Object& request = valRequest.get_obj();

                // Parse id now so errors from here on will have the id
                id = find_value(request, "id");

                // Parse method
                Value valMethod = find_value(request, "method");
                if (valMethod.type() == null_type)
                    throw JSONRPCError(-32600, "Missing method");
                if (valMethod.type() != str_type)
                    throw JSONRPCError(-32600, "Method must be a string");
                string strMethod = valMethod.get_str();
                printf("ThreadRPCServer method=%s\n", strMethod.c_str());

                // Parse params
                Value valParams = find_value(request, "params");
                Array params;
                if (valParams.type() == array_type)
                    params = valParams.get_array();
                else if (valParams.type() == null_type)
                    params = Array();
                else
                    throw JSONRPCError(-32600, "Params must be an array");

                Value result = tableRPC.execute(strMethod, params);

                // Send reply
                string strReply = JSONRPCReply(result, Value::null, id);
                stream << HTTPReply(200, strReply) << std::flush;
            }
            catch (Object& objError)
            {
                ErrorReply(stream, objError, id);
            }
            catch (std::exception& e)
            {
                ErrorReply(stream, JSONRPCError(-32700, e.what()), id);
            }
        }
    }

    json_spirit::Value CRPCTable::execute(const std::string &strMethod, const json_spirit::Array &params) const
    {
        // Find method
        const CRPCCommand *pcmd = tableRPC[strMethod];
        if (!pcmd)
            throw JSONRPCError(-32601, "Method not found");

        // Observe safe mode
        string strWarning = Core::GetWarnings("rpc");
        if (strWarning != "" && !GetBoolArg("-disablesafemode") &&
            !pcmd->okSafeMode)
            throw JSONRPCError(-2, string("Safe mode: ") + strWarning);

        try
        {
            // Execute
            Value result;
            {
                LOCK2(Core::cs_main, pwalletMain->cs_wallet);
                result = pcmd->actor(params, false);
            }
            return result;
        }
        catch (std::exception& e)
        {
            throw JSONRPCError(-1, e.what());
        }
    }


    Object CallRPC(const string& strMethod, const Array& params)
    {
        if (mapArgs["-rpcuser"] == "" && mapArgs["-rpcpassword"] == "")
            throw runtime_error(strprintf(
                _("You must set rpcpassword=<password> in the configuration file:\n%s\n"
                  "If the file does not exist, create it with owner-readable-only file permissions."),
                    GetConfigFile().string().c_str()));

        // Connect to localhost
        bool fUseSSL = GetBoolArg("-rpcssl");
        asio::io_service io_service;
        ssl::context context(ssl::context::sslv23);
        context.set_options(ssl::context::no_sslv2);
        SSLStream sslStream(io_service, context);
        SSLIOStreamDevice d(sslStream, fUseSSL);
        iostreams::stream<SSLIOStreamDevice> stream(d);
        if (!d.connect(GetArg("-rpcconnect", "127.0.0.1"), GetArg("-rpcport", CBigNum(fTestNet? TESTNET_RPC_PORT : RPC_PORT).ToString().c_str())))
            throw runtime_error("couldn't connect to server");

        // HTTP basic authentication
        string strUserPass64 = EncodeBase64(mapArgs["-rpcuser"] + ":" + mapArgs["-rpcpassword"]);
        map<string, string> mapRequestHeaders;
        mapRequestHeaders["Authorization"] = string("Basic ") + strUserPass64;

        // Send request
        string strRequest = JSONRPCRequest(strMethod, params, 1);
        string strPost = HTTPPost(strRequest, mapRequestHeaders);
        stream << strPost << std::flush;

        // Receive reply
        map<string, string> mapHeaders;
        string strReply;
        int nStatus = ReadHTTP(stream, mapHeaders, strReply);
        if (nStatus == 401)
            throw runtime_error("incorrect rpcuser or rpcpassword (authorization failed)");
        else if (nStatus >= 400 && nStatus != 400 && nStatus != 404 && nStatus != 500)
            throw runtime_error(strprintf("server returned HTTP error %d", nStatus));
        else if (strReply.empty())
            throw runtime_error("no response from server");

        // Parse reply
        Value valReply;
        if (!read_string(strReply, valReply))
            throw runtime_error("couldn't parse reply from server");
        const Object& reply = valReply.get_obj();
        if (reply.empty())
            throw runtime_error("expected reply to have result, error and id properties");

        return reply;
    }




    template<typename T>
    void ConvertTo(Value& value)
    {
        if (value.type() == str_type)
        {
            // reinterpret string as unquoted json value
            Value value2;
            if (!read_string(value.get_str(), value2))
                throw runtime_error("type mismatch");
            value = value2.get_value<T>();
        }
        else
        {
            value = value.get_value<T>();
        }
    }

    // Convert strings to command-specific RPC representation
    Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
    {
        Array params;
        BOOST_FOREACH(const std::string &param, strParams)
            params.push_back(param);

        int n = params.size();

        //
        // Special case non-string parameter types
        //
        if (strMethod == "setgenerate"            && n > 0) ConvertTo<bool>(params[0]);
        if (strMethod == "dumprichlist"           && n > 0) ConvertTo<int>(params[0]);
        if (strMethod == "setgenerate"            && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "sendtoaddress"          && n > 1) ConvertTo<double>(params[1]);
        if (strMethod == "settxfee"               && n > 0) ConvertTo<double>(params[0]);
        if (strMethod == "getreceivedbyaddress"   && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "getreceivedbyaccount"   && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "listreceivedbyaddress"  && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listreceivedbyaddress"  && n > 1) ConvertTo<bool>(params[1]);
        if (strMethod == "listreceivedbyaccount"  && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listreceivedbyaccount"  && n > 1) ConvertTo<bool>(params[1]);
        if (strMethod == "getbalance"             && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "getblockhash"           && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "getblock"               && n > 1) ConvertTo<bool>(params[1]);
        if (strMethod == "move"                   && n > 2) ConvertTo<double>(params[2]);
        if (strMethod == "move"                   && n > 3) ConvertTo<boost::int64_t>(params[3]);
        if (strMethod == "sendfrom"               && n > 2) ConvertTo<double>(params[2]);
        if (strMethod == "sendfrom"               && n > 3) ConvertTo<boost::int64_t>(params[3]);
        if (strMethod == "listtransactions"       && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "listtransactions"       && n > 2) ConvertTo<boost::int64_t>(params[2]);
        if (strMethod == "totaltransactions"      && n > 0) ConvertTo<bool>(params[0]);
        if (strMethod == "gettransactions"        && n > 1) ConvertTo<int>(params[1]);
        if (strMethod == "gettransactions"        && n > 2) ConvertTo<bool>(params[2]);
        if (strMethod == "listNTransactions"      && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listaccounts"           && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listunspent"            && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listunspent"            && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "walletpassphrase"       && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "walletpassphrase"       && n > 2) ConvertTo<bool>(params[2]);
        if (strMethod == "listsinceblock"         && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "sendmany"               && n > 1)
        {
            string s = params[1].get_str();
            Value v;
            if (!read_string(s, v) || v.type() != obj_type)
                throw runtime_error("type mismatch");
            params[1] = v.get_obj();
        }
        if (strMethod == "listunspent"           && n > 2)
        {
            string s = params[2].get_str();
            Value v;
            if (!read_string(s, v) || v.type() != array_type)
                throw runtime_error("type mismatch "+s);
            params[2] = v.get_array();
        }

        if (strMethod == "importkeys"             && n > 0)
        {
            string s = params[0].get_str();
            Value v;
            if (!read_string(s, v) || v.type() != obj_type)
                throw runtime_error("type mismatch");
            params[0] = v.get_obj();
        }

        if (strMethod == "sendmany"                && n > 2) ConvertTo<boost::int64_t>(params[2]);
        if (strMethod == "reservebalance"          && n > 0) ConvertTo<bool>(params[0]);
        if (strMethod == "reservebalance"          && n > 1) ConvertTo<double>(params[1]);
        if (strMethod == "addmultisigaddress"      && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "addmultisigaddress"      && n > 1)
        {
            string s = params[1].get_str();
            Value v;
            if (!read_string(s, v) || v.type() != array_type)
                throw runtime_error("type mismatch "+s);
            params[1] = v.get_array();
        }
        return params;
    }

    int CommandLineRPC(int argc, char *argv[])
    {
        string strPrint;
        int nRet = 0;
        try
        {
            // Skip switches
            while (argc > 1 && IsSwitchChar(argv[1][0]))
            {
                argc--;
                argv++;
            }

            // Method
            if (argc < 2)
                throw runtime_error("too few parameters");
            string strMethod = argv[1];

            // Parameters default to strings
            std::vector<std::string> strParams(&argv[2], &argv[argc]);
            Array params = RPCConvertValues(strMethod, strParams);

            // Execute
            Object reply = CallRPC(strMethod, params);

            // Parse reply
            const Value& result = find_value(reply, "result");
            const Value& error  = find_value(reply, "error");

            if (error.type() != null_type)
            {
                // Error
                strPrint = "error: " + write_string(error, false);
                int code = find_value(error.get_obj(), "code").get_int();
                nRet = abs(code);
            }
            else
            {
                // Result
                if (result.type() == null_type)
                    strPrint = "";
                else if (result.type() == str_type)
                    strPrint = result.get_str();
                else
                    strPrint = write_string(result, true);
            }
        }
        catch (std::exception& e)
        {
            strPrint = string("error: ") + e.what();
            nRet = 87;
        }
        catch (...)
        {
            PrintException(NULL, "CommandLineRPC()");
        }

        if (strPrint != "")
        {
            fprintf((nRet == 0 ? stdout : stderr), "%s\n", strPrint.c_str());
        }
        return nRet;
    }



    const CRPCTable tableRPC;

}
