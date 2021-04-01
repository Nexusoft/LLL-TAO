/* UPnP Thread. If UPnP is enabled then this thread will set up the required port forwarding. */
template <class ProtocolType>
void Server<ProtocolType>::UPnP(uint16_t nPort)
{
#ifndef USE_UPNP
    return;
#else

    if(!config::GetBoolArg("-upnp", true))
        return;

    char port[6];
    sprintf(port, "%d", nPort);

    const char * multicastif = 0;
    const char * minissdpdpath = 0;
    struct UPNPDev * devlist = 0;
    char lanaddr[64];

    #ifndef UPNPDISCOVER_SUCCESS
        /* miniupnpc 1.5 */
        devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0);
    #elif MINIUPNPC_API_VERSION < 14
        /* miniupnpc 1.6 */
        int error = 0;
        devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, &error);
    #else
        /* miniupnpc 1.9.20150730 */
        int error = 0;
        devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, 2, &error);
    #endif

    struct UPNPUrls urls;
    struct IGDdatas data;
    int nResult;

    if(devlist == 0)
    {
        debug::error(FUNCTION, "No UPnP devices found");
        return;
    }

    nResult = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
    if (nResult == 1)
    {

        std::string strDesc = version::CLIENT_VERSION_BUILD_STRING;
    #ifndef UPNPDISCOVER_SUCCESS
            /* miniupnpc 1.5 */
            nResult = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                port, port, lanaddr, strDesc.c_str(), "TCP", 0);
    #else
            /* miniupnpc 1.6 */
            nResult = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                port, port, lanaddr, strDesc.c_str(), "TCP", 0, "0");
    #endif

        if(nResult != UPNPCOMMAND_SUCCESS)
            debug::error(FUNCTION, "AddPortMapping(", port, ", ", port, ", ", lanaddr, ") failed with code ", nResult, " (", strupnperror(nResult), ")");
        else
            debug::log(1, "UPnP Port Mapping successful for port: ", nPort);

        runtime::timer TIMER;
        TIMER.Start();

        while(!config::fShutdown.load())
        {
            if (TIMER.Elapsed() >= 600) // Refresh every 10 minutes
            {
                TIMER.Reset();

    #ifndef UPNPDISCOVER_SUCCESS
                /* miniupnpc 1.5 */
                nResult = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                    port, port, lanaddr, strDesc.c_str(), "TCP", 0);
    #else
                /* miniupnpc 1.6 */
                nResult = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                                    port, port, lanaddr, strDesc.c_str(), "TCP", 0, "0");
    #endif

                if(nResult != UPNPCOMMAND_SUCCESS)
                    debug::error(FUNCTION, "AddPortMapping(", port, ", ", port, ", ", lanaddr, ") failed with code ", nResult, " (", strupnperror(nResult), ")");
                else
                    debug::log(1, "UPnP Port Mapping successful for port: ", nPort);
            }
            runtime::sleep(2000);
        }

        /* Shutdown sequence */
        nResult = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port, "TCP", 0);
        debug::log(1, "UPNP_DeletePortMapping() returned : ", nResult);
        freeUPNPDevlist(devlist); devlist = 0;
        FreeUPNPUrls(&urls);
    }
    else
    {
        debug::error(FUNCTION, "No valid UPnP IGDs found.");
        freeUPNPDevlist(devlist); devlist = 0;
        if (nResult != 0)
            FreeUPNPUrls(&urls);

        return;
    }

   debug::log(0, "UPnP closed.");
#endif
}
