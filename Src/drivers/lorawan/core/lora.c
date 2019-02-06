/******************************************************************************
  * @file    lora.c
  * @author  MCD Application Team
  * @brief   lora API to drive the lora state Machine
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include <it_sdk/itsdk.h>
#if ITSDK_WITH_LORAWAN_LIB == __ENABLE
#include <it_sdk/logger/logger.h>

/* Includes ------------------------------------------------------------------*/
#include <drivers/sx1276/hw.h>
#include <drivers/lorawan/timeServer.h>
#include <drivers/lorawan/mac/LoRaMac.h>
#include <drivers/lorawan/core/lora.h>
#include <drivers/lorawan/core/lora-test.h>
#include <drivers/lorawan/compiled_region.h>


/*!
 *  Select either Device_Time_req or Beacon_Time_Req following LoRaWAN version 
 *  - Device_Time_req   Available for V1.0.3 or later                          
 *  - Beacon_time_Req   Available for V1.0.2 and before                        
 */
#define USE_DEVICE_TIMING

/*!
 * Join requests trials duty cycle.
 */
#define OVER_THE_AIR_ACTIVATION_DUTYCYCLE           10000  // 10 [s] value in ms

#if defined( REGION_EU868 )

#include <drivers/lorawan/mac/LoRaMacTest.h>

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        true
#endif
/*!
 * Default ping slots periodicity
 *
 * \remark periodicity is equal to 2^LORAWAN_DEFAULT_PING_SLOT_PERIODICITY seconds
 *         example: 2^3 = 8 seconds. The end-device will open an Rx slot every 8 seconds.
 */
#define LORAWAN_DEFAULT_PING_SLOT_PERIODICITY       0   

#define HEX16(X)  X[0],X[1], X[2],X[3], X[4],X[5], X[6],X[7],X[8],X[9], X[10],X[11], X[12],X[13], X[14],X[15]
#define HEX8(X)   X[0],X[1], X[2],X[3], X[4],X[5], X[6],X[7]
/*
static uint8_t DevEui[] = ITSDK_LORAWAN_DEVEUI;
static uint8_t JoinEui[] = ITSDK_LORAWAN_APPEUI;
static uint8_t AppKey[] = ITSDK_LORAWAN_APPKEY;
static uint8_t NwkKey[] = ITSDK_LORAWAN_NWKKEY;
*/
static MlmeReqJoin_t JoinParameters;

#if( ITSDK_LORAWAN_ACTIVATION == __LORAWAN_ABP )
/*
static uint8_t FNwkSIntKey[] = ITSDK_LORAWAN_FNWKSKEY;
static uint8_t SNwkSIntKey[] = ITSDK_LORAWAN_SNWKSKEY;
static uint8_t NwkSEncKey[] = ITSDK_LORAWAN_NWKSKEY;
static uint8_t AppSKey[] = ITSDK_LORAWAN_APPSKEY;
static uint32_t DevAddr = ITSDK_LORAWAN_DEVADDR;
*/
#endif

#ifdef LORAMAC_CLASSB_ENABLED
static LoraErrorStatus LORA_BeaconReq( void);
static LoraErrorStatus LORA_PingSlotReq( void);

#if defined( USE_DEVICE_TIMING )
static LoraErrorStatus LORA_DeviceTimeReq(void);
#else
static LoraErrorStatus LORA_BeaconTimeReq(void);
#endif /* USE_DEVICE_TIMING */
#endif /* LORAMAC_CLASSB_ENABLED */

/*!
 * Defines the LoRa parameters at Init
 */
/*
static LoRaParam_t* LoRaParamInit;
static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;
static MibRequestConfirm_t mibReq;

static LoRaMainCallback_t *LoRaMainCallbacks;
*/

/*!
 * MAC event info status strings.
 */
/*
const char* _EventInfoStatusStrings[] =
{ 
    "OK", "Error", "Tx timeout", "Rx 1 timeout",
    "Rx 2 timeout", "Rx1 error", "Rx2 error",
    "Join failed", "Downlink repeated", "Tx DR payload size error",
    "Downlink too many frames loss", "Address fail", "MIC fail",
    "Multicast faile", "Beacon locked", "Beacon lost", "Beacon not found"
};
*/
/*!
 * MAC status strings
 */
/*
const char* _MacStatusStrings[] =
{
    "OK", "Busy", "Service unknown", "Parameter invalid", "Frequency invalid",
    "Datarate invalid", "Freuqency or datarate invalid", "No network joined",
    "Length error", "Device OFF", "Region not supported", "Skipped APP data",
    "DutyC restricted", "No channel found", "No free channel found",
    "Busy beacon reserved time", "Busy ping-slot window time",
    "Busy uplink collision", "Crypto error", "FCnt handler error",
    "MAC command error", "ERROR"
};
*/
/*
const char* _MlmeReqStrings[] =
{
    "MLME_JOIN",
    "MLME_REJOIN_0",
    "MLME_REJOIN_1",
    "MLME_LINK_CHECK",
    "MLME_TXCW",
    "MLME_TXCW_1",
    "MLME_SCHEDULE_UPLINK",
    "MLME_NVM_CTXS_UPDATE",
    "MLME_DERIVE_MC_KE_KEY",
    "MLME_DERIVE_MC_KEY_PAIR",
    "MLME_DEVICE_TIME",
    "MLME_BEACON",
    "MLME_BEACON_ACQUISITION",
    "MLME_PING_SLOT_INFO",
    "MLME_BEACON_TIMING,MLME_BEACON_LOST"
};
*/
 void TraceUpLinkFrame(McpsConfirm_t *mcpsConfirm);
 void TraceDownLinkFrame(McpsIndication_t *mcpsIndication);
#ifdef LORAMAC_CLASSB_ENABLED
static void TraceBeaconInfo(MlmeIndication_t *mlmeIndication);
#endif /* LORAMAC_CLASSB_ENABLED */

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] McpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
/*
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{

    TVL2( PRINTNOW(); PRINTF("APP> McpsConfirm STATUS: %s\r\n", _EventInfoStatusStrings[mcpsConfirm->Status] ); )

    if( mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
    {
        switch( mcpsConfirm->McpsRequest )
        {
            case MCPS_UNCONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                break;
            }
            case MCPS_CONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                // Check AckReceived
            	if(mcpsConfirm->AckReceived){
            	     LoRaMainCallbacks->LORA_McpsDataConfirm();
            	}
                // Check NbTrials
                break;
            }
            case MCPS_PROPRIETARY:
            {
                break;
            }
            default:
                break;
        }
    }
    
    //implicitely desactivated when VERBOSE_LEVEL < 2
    TraceUpLinkFrame(mcpsConfirm);

}
*/
/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
/*
static void McpsIndication( McpsIndication_t *mcpsIndication )
{
    TVL2( PRINTNOW(); PRINTF("APP> McpsInd STATUS: %s\r\n", _EventInfoStatusStrings[mcpsIndication->Status] );)

    lora_AppData_t _AppData;
    if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
    {
        return;
    }

    switch( mcpsIndication->McpsIndication )
    {
        case MCPS_UNCONFIRMED:
        {
            break;
        }
        case MCPS_CONFIRMED:
        {
            break;
        }
        case MCPS_PROPRIETARY:
        {
            break;
        }
        case MCPS_MULTICAST:
        {
            break;
        }
        default:
            break;
    }

    // Check Multicast
    // Check Port
    // Check Datarate
    // Check FramePending
    if( mcpsIndication->FramePending == true )
    {
        // The server signals that it has pending data to be sent.
        // We schedule an uplink as soon as possible to flush the server.
        LoRaMainCallbacks->LORA_TxNeeded( );
    }
    // Check Buffer
    // Check BufferSize
    // Check Rssi
    // Check Snr
    // Check RxSlot
    if (certif_running() == true )
    {
      certif_DownLinkIncrement( );
    }

    if( mcpsIndication->RxData == true )
    {
      switch( mcpsIndication->Port )
      {
        case CERTIF_PORT:
          certif_rx( mcpsIndication, &JoinParameters );
          break;
        default:
          
          _AppData.Port = mcpsIndication->Port;
          _AppData.BuffSize = mcpsIndication->BufferSize;
          _AppData.Buff = mcpsIndication->Buffer;
        
          LoRaMainCallbacks->LORA_RxData( &_AppData );
          break;
      }
    }
    
    //implicitely desactivated when VERBOSE_LEVEL < 2
    TraceDownLinkFrame(mcpsIndication);
}
*/
/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] MlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
/*
static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
#ifdef LORAMAC_CLASSB_ENABLED
    MibRequestConfirm_t mibReq;
#endif // LORAMAC_CLASSB_ENABLED

    TVL2( PRINTNOW(); PRINTF("APP> MlmeConfirm STATUS: %s\r\n", _EventInfoStatusStrings[mlmeConfirm->Status] );)
    
    switch( mlmeConfirm->MlmeRequest )
    {
        case MLME_JOIN:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
              // Status is OK, node has joined the network
              LoRaMainCallbacks->LORA_HasJoined();

#ifdef LORAMAC_CLASSB_ENABLED
#if defined( USE_DEVICE_TIMING )              
              LORA_DeviceTimeReq();
#else              
              LORA_BeaconTimeReq();
#endif // USE_DEVICE_TIMING
#endif // LORAMAC_CLASSB_ENABLED
            }
            else
            {
                // Join was not successful. Try to join again
                LORA_Join();
            }
            break;
        }
        case MLME_LINK_CHECK:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Check DemodMargin
                // Check NbGateways
                if (certif_running() == true )
                {
                     certif_linkCheck( mlmeConfirm);
                }
            }
            break;
        }
#ifdef LORAMAC_CLASSB_ENABLED
        case MLME_BEACON_ACQUISITION:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Beacon has been acquired
                // REquest Server for Ping Slot
                LORA_PingSlotReq( );
            }
            else
            {
                // Beacon not acquired
                // Search again
                // we can check if the MAC has received a time reference for the beacon
                // in this case do either a Device_Time_Req  or a Beacon_Timing_req
                LORA_BeaconReq( );
            }
            break;
        }
        case MLME_PING_SLOT_INFO:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
               // class B is now ativated
                mibReq.Type = MIB_DEVICE_CLASS;
                mibReq.Param.Class = CLASS_B;
                LoRaMacMibSetRequestConfirm( &mibReq );
                
#if defined( REGION_AU915 ) || defined( REGION_US915 )
                mibReq.Type = MIB_PING_SLOT_DATARATE;
                mibReq.Param.PingSlotDatarate = DR_8;
                LoRaMacMibSetRequestConfirm( &mibReq );
#endif
                TVL2( PRINTF("\r\n#= Switch to Class B done. =#r\n" );)
                
                //notify upper layer
                LoRaMainCallbacks->LORA_ConfirmClass(CLASS_B);
            }
            else
            {
                LORA_PingSlotReq( );
            }
            break;
        }
#if defined( USE_DEVICE_TIMING )        
        case MLME_DEVICE_TIME:
        {        
            if( mlmeConfirm->Status != LORAMAC_EVENT_INFO_STATUS_OK )
            {
              LORA_DeviceTimeReq();
            }  
        }              
#endif // USE_DEVICE_TIMING
#endif // LORAMAC_CLASSB_ENABLED
        default:
            break;
    }
}
*/
/*!
 * \brief   MLME-Indication event function
 *
 * \param   [IN] MlmeIndication - Pointer to the indication structure.
 */
/*
static void MlmeIndication( MlmeIndication_t *MlmeIndication )
{
#ifdef LORAMAC_CLASSB_ENABLED
    MibRequestConfirm_t mibReq;
#endif // LORAMAC_CLASSB_ENABLED

    TVL2( PRINTNOW(); PRINTF("APP> MLMEInd STATUS: %s\r\n", _EventInfoStatusStrings[MlmeIndication->Status] );    )

    switch( MlmeIndication->MlmeIndication )
    {
        case MLME_SCHEDULE_UPLINK:
        {
            // The MAC signals that we shall provide an uplink as soon as possible
            LoRaMainCallbacks->LORA_TxNeeded( );			
            break;
        }
#ifdef LORAMAC_CLASSB_ENABLED
        case MLME_BEACON_LOST:
        {
            // Switch to class A again
            mibReq.Type = MIB_DEVICE_CLASS;
            mibReq.Param.Class = CLASS_A;
            LoRaMacMibSetRequestConfirm( &mibReq );
            
            TVL2( PRINTF("\r\n#= Switch to Class A done. =# BEACON LOST\r\n" ); )

            LORA_BeaconReq();
            break;
        }
        case MLME_BEACON:
        {
            if( MlmeIndication->Status == LORAMAC_EVENT_INFO_STATUS_BEACON_LOCKED )
            {
              TraceBeaconInfo(MlmeIndication);
            }
            else
            {
              TVL2( PRINTF( "BEACON NOT RECEIVED\n\r");)
            }
            break;

        }
#endif // LORAMAC_CLASSB_ENABLED
        default:
            break;
    }
}
*/
/**
 *  lora Init
 */
/*
void LORA_Init (LoRaMainCallback_t *callbacks, LoRaParam_t* LoRaParam, uint16_t region )
{
  // init the Tx Duty Cycle
  LoRaParamInit = LoRaParam;
  
  / init the main call backs
  LoRaMainCallbacks = callbacks;
  
#if (ITSDK_LORAWAN_DEVEUI_SRC == __LORAWAN_DEVEUI_GENERATED)
  LoRaMainCallbacks->BoardGetUniqueId( DevEui );  
#endif
  
  if ( LoRaParamInit->JoinType == __LORAWAN_OTAA ) {
	  PPRINTF( "OTAA\n\r");
	  PPRINTF( "DevEui= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(LoRaParamInit->devEui));
	  PPRINTF( "AppEui= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(LoRaParamInit->config.otaa.appEui));
	  PPRINTF( "AppKey= %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\r", HEX16(LoRaParamInit->config.otaa.appKey));
  } else if (LoRaParamInit->JoinType == __LORAWAN_ABP) {
	#if (STATIC_DEVICE_ADDRESS != 1)
	  // Random seed initialization
	  srand1( LoRaMainCallbacks->BoardGetRandomSeed( ) );
	  // Choose a random device address
	  LoRaParamInit->config.abp.devAddr = randr( 0, 0x01FFFFFF );
	#endif
	PPRINTF( "ABP\n\r");
	PPRINTF( "DevEui= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(LoRaParamInit->devEui));
	PPRINTF( "DevAdd=  %08X\n\r", LoRaParamInit->config.abp.devAddr) ;
	PPRINTF( "NwkSKey= %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\r", HEX16(LoRaParamInit->config.abp.nwkSEncKey));
	PPRINTF( "AppSKey= %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\r", HEX16(LoRaParamInit->config.abp.appSKey));
  }

  LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
  LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
  LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
  LoRaMacPrimitives.MacMlmeIndication = MlmeIndication;
  LoRaMacCallbacks.GetBatteryLevel = LoRaMainCallbacks->BoardGetBatteryLevel;
  LoRaMacCallbacks.GetTemperatureLevel = LoRaMainCallbacks->BoardGetTemperatureLevel;
  LoRaMacCallbacks.MacProcessNotify = LoRaMainCallbacks->MacProcessNotify;
  switch ( region ) {
#if defined( REGION_AS923 )
        case __LORAWAN_REGION_AS923:
        	LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AS923 );
        	break;
#endif
#if defined( REGION_AU915 )
        case __LORAWAN_REGION_AU915:
            LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AU915 );
            break;
#endif
#if defined( REGION_CN470 )
        case __LORAWAN_REGION_CN470:
        	LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_CN470 );
        	break;
#endif
#if defined( REGION_CN779 )
        case __LORAWAN_REGION_CN779:
            LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_CN779 );
            break;
#endif
#if defined( REGION_EU433 )
        case __LORAWAN_REGION_EU433:
        	LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU433 );
        	break;
#endif
#if defined( REGION_EU868 )
        case __LORAWAN_REGION_EU868:
        	LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU868 );
        	break;
#endif
#if defined( REGION_KR920 )
        case __LORAWAN_REGION_KR920:
        	LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_KR920 );
        	break;
#endif
#if defined( REGION_IN865 )
        case __LORAWAN_REGION_IN865:
        	LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_IN865 );
        	break;
#endif
#if defined( REGION_US915 )
        case __LORAWAN_REGION_US915:
        	break;
#endif
#if defined( REGION_RU864 )
        case __LORAWAN_REGION_RU864:
        	LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_RU864 );
        	break;
#endif
        default:
        	return;
        }

#if ITSDK_LORAWAN_REGION_ALLOWED == __LORAWAN_REGION_NONE
	#error "Please define a region in the compiler options."
#endif


#if defined( HYBRID )
                uint16_t channelMask[] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000};
                mibReq.Type = MIB_CHANNELS_MASK;
                mibReq.Param.ChannelsMask = channelMask;
                LoRaMacMibSetRequestConfirm( &mibReq );
                mibReq.Type = MIB_CHANNELS_DEFAULT_MASK;
                mibReq.Param.ChannelsDefaultMask = channelMask;
                LoRaMacMibSetRequestConfirm( &mibReq );
#endif
      
      mibReq.Type = MIB_ADR;
      mibReq.Param.AdrEnable = LoRaParamInit->AdrEnable;
      LoRaMacMibSetRequestConfirm( &mibReq );

      mibReq.Type = MIB_PUBLIC_NETWORK;
      mibReq.Param.EnablePublicNetwork = LoRaParamInit->EnablePublicNetwork;
      LoRaMacMibSetRequestConfirm( &mibReq );

      if ( LoRaParamInit->JoinType == __LORAWAN_OTAA ) {
		  mibReq.Type = MIB_APP_KEY;
		  mibReq.Param.AppKey = LoRaParamInit->config.otaa.appKey;
		  LoRaMacMibSetRequestConfirm( &mibReq );

		  mibReq.Type = MIB_NWK_KEY;
		  mibReq.Param.NwkKey = LoRaParamInit->config.otaa.nwkKey;
		  LoRaMacMibSetRequestConfirm( &mibReq );
      } else if (LoRaParamInit->JoinType == __LORAWAN_ABP) {
    	  mibReq.Type = MIB_NET_ID;
    	  mibReq.Param.NetID = ITSDK_LORAWAN_NETWORKID;
    	  LoRaMacMibSetRequestConfirm( &mibReq );

    	  mibReq.Type = MIB_DEV_ADDR;
    	  mibReq.Param.DevAddr = LoRaParamInit->config.abp.devAddr;
    	  LoRaMacMibSetRequestConfirm( &mibReq );

    	  mibReq.Type = MIB_F_NWK_S_INT_KEY;
    	  mibReq.Param.FNwkSIntKey = LoRaParamInit->config.abp.FNwkSIntKey;
    	  LoRaMacMibSetRequestConfirm( &mibReq );

    	  mibReq.Type = MIB_S_NWK_S_INT_KEY;
    	  mibReq.Param.SNwkSIntKey = LoRaParamInit->config.abp.SNwkSIntKey;
    	  LoRaMacMibSetRequestConfirm( &mibReq );

    	  mibReq.Type = MIB_NWK_S_ENC_KEY;
    	  mibReq.Param.NwkSEncKey = LoRaParamInit->config.abp.nwkSEncKey;
    	  LoRaMacMibSetRequestConfirm( &mibReq );

    	  mibReq.Type = MIB_APP_S_KEY;
    	  mibReq.Param.AppSKey = LoRaParamInit->config.abp.appSKey;
    	  LoRaMacMibSetRequestConfirm( &mibReq );

    	  mibReq.Type = MIB_NETWORK_ACTIVATION;
    	  mibReq.Param.NetworkActivation = ACTIVATION_TYPE_ABP;
    	  LoRaMacMibSetRequestConfirm( &mibReq );
      }


      mibReq.Type = MIB_DEVICE_CLASS;
      mibReq.Param.Class= CLASS_A;
      LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( REGION_EU868 )
      if ( region == __LORAWAN_REGION_EU868 ) {
    	  LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );
      }
#endif

      mibReq.Type = MIB_SYSTEM_MAX_RX_ERROR;
      mibReq.Param.SystemMaxRxError = 20;
      LoRaMacMibSetRequestConfirm( &mibReq );

      //set Mac statein Idle
      LoRaMacStart( );

}
*/

/*
void LORA_Join( void)
{
    MlmeReq_t mlmeReq;
  
    mlmeReq.Type = MLME_JOIN;
    mlmeReq.Req.Join.DevEui = LoRaParamInit->devEui;
    mlmeReq.Req.Join.JoinEui = LoRaParamInit->config.otaa.appEui;
    mlmeReq.Req.Join.Datarate = LoRaParamInit->TxDatarate;
  
    //JoinParameters = mlmeReq.Req.Join;

    if (LoRaParamInit->JoinType ==  __LORAWAN_OTAA) {
        LoRaMacStatus_t r = LoRaMacMlmeRequest( &mlmeReq );
        if ( r != LORAMAC_STATUS_OK ) {
        	log_warn("LoRaMacMlmeRequest return error(%d)\r\n",r);
        }


    } else if(LoRaParamInit->JoinType ==  __LORAWAN_ABP) {
        // Enable legacy mode to operate according to LoRaWAN Spec. 1.0.3
        Version_t abpLrWanVersion;

        abpLrWanVersion.Fields.Major    = 1;
        abpLrWanVersion.Fields.Minor    = 0;
        abpLrWanVersion.Fields.Revision = 3;
        abpLrWanVersion.Fields.Rfu      = 0;

        mibReq.Type = MIB_ABP_LORAWAN_VERSION;
        mibReq.Param.AbpLrWanVersion = abpLrWanVersion;
        LoRaMacMibSetRequestConfirm( &mibReq );

        LoRaMainCallbacks->LORA_HasJoined();
    }
    
}
*/
/*
LoraFlagStatus LORA_JoinStatus( void)
{
  MibRequestConfirm_t mibReq;

  mibReq.Type = MIB_NETWORK_ACTIVATION;
  
  LoRaMacMibGetRequestConfirm( &mibReq );

  if( mibReq.Param.NetworkActivation == ACTIVATION_TYPE_NONE )
  {
    return LORA_RESET;
  }
  else
  {
    return LORA_SET;
  }
}
*/

/*
bool LORA_send(lora_AppData_t* AppData, LoraConfirm_t IsTxConfirmed)
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;
  
    //if certification test are on going, application data is not sent
    if (certif_running() == true)
    {
      return false;
    }
    
    if( LoRaMacQueryTxPossible( AppData->BuffSize, &txInfo ) != LORAMAC_STATUS_OK )
    {
    	// a voir ce truc.. un peu etrange ...
    	// on dirait que l'on regarde si busy ou pas...
    	// ici cas busy

        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = LoRaParamInit->TxDatarate;
    }
    else
    {
        if( IsTxConfirmed == LORAWAN_UNCONFIRMED_MSG )
        {
        	log_info("### send type unconfirmed\r\n");

            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = AppData->Port;
            mcpsReq.Req.Unconfirmed.fBufferSize = AppData->BuffSize;
            mcpsReq.Req.Unconfirmed.fBuffer = AppData->Buff;
            mcpsReq.Req.Unconfirmed.Datarate = LoRaParamInit->TxDatarate;
        }
        else
        {
        	log_info("### send type confirmed\r\n");

        	mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = AppData->Port;
            mcpsReq.Req.Confirmed.fBufferSize = AppData->BuffSize;
            mcpsReq.Req.Confirmed.fBuffer = AppData->Buff;
            mcpsReq.Req.Confirmed.NbTrials = 8;
            mcpsReq.Req.Confirmed.Datarate = LoRaParamInit->TxDatarate;
        }
    }
    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
    {
        return false;
    }
    return true;
}  
*/
/*
#ifdef LORAMAC_CLASSB_ENABLED
#if defined( USE_DEVICE_TIMING )
static LoraErrorStatus LORA_DeviceTimeReq( void)
{
  MlmeReq_t mlmeReq;

  mlmeReq.Type = MLME_DEVICE_TIME;

  if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
  {
    return LORA_SUCCESS;
  }
  else
  {
    return LORA_ERROR;
  }
}
#else
static LoraErrorStatus LORA_BeaconTimeReq( void)
{
  MlmeReq_t mlmeReq;

  mlmeReq.Type = MLME_BEACON_TIMING;

  if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
  {
    return LORA_SUCCESS;
  }
  else
  {
    return LORA_ERROR;
  }
}
#endif

static LoraErrorStatus LORA_BeaconReq( void)
{
  MlmeReq_t mlmeReq;

  mlmeReq.Type = MLME_BEACON_ACQUISITION;

  if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
  {
    return LORA_SUCCESS;
  }
  else
  {
    return LORA_ERROR;
  }
}

static LoraErrorStatus LORA_PingSlotReq( void)
{

  MlmeReq_t mlmeReq;

  mlmeReq.Type = MLME_LINK_CHECK;
  LoRaMacMlmeRequest( &mlmeReq );

  mlmeReq.Type = MLME_PING_SLOT_INFO;
  mlmeReq.Req.PingSlotInfo.PingSlot.Fields.Periodicity = LORAWAN_DEFAULT_PING_SLOT_PERIODICITY;
  mlmeReq.Req.PingSlotInfo.PingSlot.Fields.RFU = 0;

  if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
  {
      return LORA_SUCCESS;
  }
  else
  {
     return LORA_ERROR;
  }
}
#endif // LORAMAC_CLASSB_ENABLED
*/
/*
LoraErrorStatus LORA_RequestClass( DeviceClass_t newClass )
{
  LoraErrorStatus Errorstatus = LORA_SUCCESS;
  MibRequestConfirm_t mibReq;
  DeviceClass_t currentClass;
  
  mibReq.Type = MIB_DEVICE_CLASS;
  LoRaMacMibGetRequestConfirm( &mibReq );
  
  currentClass = mibReq.Param.Class;
  //attempt to swicth only if class update
  if (currentClass != newClass)
  {
    switch (newClass)
    {
      case CLASS_A:
      {
        mibReq.Param.Class = CLASS_A;
        if( LoRaMacMibSetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK )
        {
        //switch is instantanuous
          LoRaMainCallbacks->LORA_ConfirmClass(CLASS_A);
        }
        else
        {
          Errorstatus = LORA_ERROR;
        }
        break;
      }
      case CLASS_B:
      {
#ifdef LORAMAC_CLASSB_ENABLED
        if (currentClass != CLASS_A)
        {
          Errorstatus = LORA_ERROR;
        }
        //switch is not instantanuous
        Errorstatus = LORA_BeaconReq( );
#else
        PRINTF( "warning: LORAMAC_CLASSB_ENABLED has not been defined at compilation\n\r");
#endif // LORAMAC_CLASSB_ENABLED
        break;
      }
      case CLASS_C:
      {
        if (currentClass != CLASS_A)
        {
          Errorstatus = LORA_ERROR;
        }
        //switch is instantanuous
        mibReq.Param.Class = CLASS_C;
        if( LoRaMacMibSetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK )
        {
          LoRaMainCallbacks->LORA_ConfirmClass(CLASS_C);
        }
        else
        {
            Errorstatus = LORA_ERROR;
        }
        break;
      }
      default:
        break;
    } 
  }
  return Errorstatus;
}
*/
/*
void LORA_GetCurrentClass( DeviceClass_t *currentClass )
{
  MibRequestConfirm_t mibReq;
  
  mibReq.Type = MIB_DEVICE_CLASS;
  LoRaMacMibGetRequestConfirm( &mibReq );
  
  *currentClass = mibReq.Param.Class;
}
*/



/**
 * @TODO bug la dedans il y a une ref aAppData qui n'est pas global !!!
 */
void TraceUpLinkFrame(McpsConfirm_t *mcpsConfirm)
{

    MibRequestConfirm_t mibGet;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_DEVICE_CLASS;
    LoRaMacMibGetRequestConfirm( &mibReq );
  
    TVL2( PRINTF("\r\n" );)
    TVL2( PRINTNOW(); PRINTF("#= U/L FRAME %lu =# Class %c, Port %d, data size %d, pwr %d, ", \
                             mcpsConfirm->UpLinkCounter, \
                             "ABC"[mibReq.Param.Class], \
                             0, /*AppData.Port,*/ \
                             0, /*AppData.BuffSize,*/ \
                             mcpsConfirm->TxPower );)

    mibGet.Type  = MIB_CHANNELS_MASK;
    if( LoRaMacMibGetRequestConfirm( &mibGet ) == LORAMAC_STATUS_OK )
    {
        TVL2( PRINTF( "Channel Mask ");)
#if defined( REGION_AS923 ) || defined( REGION_CN779 ) || \
    defined( REGION_EU868 ) || defined( REGION_IN865 ) || \
    defined( REGION_KR920 ) || defined( REGION_EU433 ) || \
    defined( REGION_RU864 )

        for( uint8_t i = 0; i < 1; i++)

#elif defined( REGION_AU915 ) || defined( REGION_US915 ) || defined( REGION_CN470 )

        for( uint8_t i = 0; i < 5; i++)
#else

#error "Please define a region in the compiler options."

#endif
        {
            TVL2( PRINTF( "%04X ", mibGet.Param.ChannelsMask[i] );)
        }
    }

    TVL2( PRINTF("\r\n\r\n" );)
} 


void TraceDownLinkFrame(McpsIndication_t *mcpsIndication)
{
    const char *slotStrings[] = { "1", "2", "C", "Ping-Slot", "Multicast Ping-Slot" };
  
    TVL2( PRINTF("\r\n" );)
    TVL2( PRINTNOW(); PRINTF("#= D/L FRAME %lu =# RxWin %s, Port %d, data size %d, rssi %d, snr %d\r\n\r\n", \
                             mcpsIndication->DownLinkCounter, \
                             slotStrings[mcpsIndication->RxSlot], \
                             mcpsIndication->Port, \
                             mcpsIndication->BufferSize, \
                             mcpsIndication->Rssi, \
                             mcpsIndication->Snr );)
}  

#ifdef LORAMAC_CLASSB_ENABLED
static void TraceBeaconInfo(MlmeIndication_t *mlmeIndication)
{
    int32_t snr = 0;
    if( mlmeIndication->BeaconInfo.Snr & 0x80 ) // The SNR sign bit is 1
    {
        // Invert and divide by 4
        snr = ( ( ~mlmeIndication->BeaconInfo.Snr + 1 ) & 0xFF ) >> 2;
        snr = -snr;
    }
    else
    {
        // Divide by 4
        snr = ( mlmeIndication->BeaconInfo.Snr & 0xFF ) >> 2;
    }  
    
    TVL2( PRINTF("\r\n" );)
    TVL2( PRINTNOW(); PRINTF("#= BEACON %lu =#, GW desc %d, rssi %d, snr %ld\r\n\r\n", \
                             mlmeIndication->BeaconInfo.Time, \
                             mlmeIndication->BeaconInfo.GwSpecific.InfoDesc, \
                             mlmeIndication->BeaconInfo.Rssi, \
                             snr );)
}
#endif /* LORAMAC_CLASSB_ENABLED */

#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
