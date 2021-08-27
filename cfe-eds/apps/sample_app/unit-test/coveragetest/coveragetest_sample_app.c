/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/*
** File: coveragetest_sample_app.c
**
** Purpose:
** Coverage Unit Test cases for the SAMPLE Application
**
** Notes:
** This implements various test cases to exercise all code
** paths through all functions defined in the SAMPLE application.
**
** It is primarily focused at providing examples of the various
** stub configurations, hook functions, and wrapper calls that
** are often needed when coercing certain code paths through
** complex functions.
*/


/*
 * Includes
 */

#include "sample_lib.h"
#include "sample_app_coveragetest_common.h"
#include "ut_sample_app.h"

/* to get the SAMPLE_Function() declaration */

typedef struct
{
    uint16 ExpectedEvent;
    uint32 MatchCount;
    const char *ExpectedText;
} UT_CheckEvent_t;

/*
 * Stub object for EDS registration call
 */
const struct EdsLib_App_DataTypeDB { uint32 Data; } SAMPLE_APP_DATATYPE_DB = { 0 };


/*
 * An example hook function to check for a specific event.
 */
static int32 UT_CheckEvent_Hook(void *UserObj, int32 StubRetcode,
        uint32 CallCount, const UT_StubContext_t *Context, va_list va)
{
    UT_CheckEvent_t *State = UserObj;
    char TestText[CFE_MISSION_EVS_MAX_MESSAGE_LENGTH];
    uint16 EventId;
    const char *Spec;

    /*
     * The CFE_EVS_SendEvent stub passes the EventID as the
     * first context argument.
     */
    if (Context->ArgCount > 0)
    {
        EventId = UT_Hook_GetArgValueByName(Context, "EventID", uint16);
        if (EventId == State->ExpectedEvent)
        {
            /*
             * Example of how to validate the full argument set.
             * If reference text was supplied, also check against this.
             *
             * NOTE: While this can be done, use with discretion - This isn't really
             * verifying that the FSW code unit generated the correct event text,
             * rather it is validating what the system snprintf() library function
             * produces when passed the format string and args.
             *
             * __This derived string is not an actual output of the unit under test__
             */
            if (State->ExpectedText != NULL)
            {
                Spec = UT_Hook_GetArgValueByName(Context, "Spec", const char *);
                if (Spec != NULL)
                {
                    vsnprintf(TestText, sizeof(TestText), Spec, va);
                    if (strcmp(TestText,State->ExpectedText) == 0)
                    {
                        ++State->MatchCount;
                    }
                }
            }
            else
            {
                ++State->MatchCount;
            }
        }
    }

    return 0;
}

/*
 * Helper function to set up for event checking
 * This attaches the hook function to CFE_EVS_SendEvent
 */
static void UT_CheckEvent_Setup(UT_CheckEvent_t *Evt, uint16 ExpectedEvent, const char *ExpectedText)
{
    memset(Evt, 0, sizeof(*Evt));
    Evt->ExpectedEvent = ExpectedEvent;
    Evt->ExpectedText = ExpectedText;
    UT_SetVaHookFunction(UT_KEY(CFE_EVS_SendEvent), UT_CheckEvent_Hook, Evt);
}



/*
**********************************************************************************
**          TEST CASE FUNCTIONS
**********************************************************************************
*/

void Test_SAMPLE_AppMain(void)
{
    /*
     * Test Case For:
     * void SAMPLE_AppMain( void )
     */

    UT_CheckEvent_t EventTest;

    /*
     * SAMPLE_AppMain does not return a value,
     * but it has several internal decision points
     * that need to be exercised here.
     *
     * First call it in "nominal" mode where all
     * dependent calls should be successful by default.
     */
    SAMPLE_AppMain();

    /*
     * Confirm that CFE_ES_ExitApp() was called at the end of execution
     */
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_ES_ExitApp)) == 1,
            "CFE_ES_ExitApp() called");

    /*
     * Now set up individual cases for each of the error paths.
     * The first is for SAMPLE_AppInit().  As this is in the same
     * code unit, it is not a stub where the return code can be
     * easily set.  In order to get this to fail, an underlying
     * call needs to fail, and the error gets propagated through.
     * The call to CFE_EVS_Register is the first opportunity.
     * Any identifiable (non-success) return code should work.
     */
    UT_SetDeferredRetcode(UT_KEY(CFE_EVS_Register), 1,
            CFE_EVS_INVALID_PARAMETER);

    /*
     * Just call the function again.  It does not return
     * the value, so there is nothing to test for here directly.
     * However, it should show up in the coverage report that
     * the SAMPLE_AppInit() failure path was taken.
     */
    SAMPLE_AppMain();

    /*
     * This can validate that the internal "RunStatus" was
     * set to CFE_ES_RunStatus_APP_ERROR, by querying the struct directly.
     *
     * It is always advisable to include the _actual_ values
     * when asserting on conditions, so if/when it fails, the
     * log will show what the incorrect value was.
     */
    UtAssert_True(SAMPLE_AppData.RunStatus == CFE_ES_RunStatus_APP_ERROR,
            "SAMPLE_AppData.RunStatus (%lu) == CFE_ES_RunStatus_APP_ERROR",
            (unsigned long)SAMPLE_AppData.RunStatus);


    /*
     * Note that CFE_ES_RunLoop returns a boolean value,
     * so in order to exercise the internal "while" loop,
     * it needs to return TRUE.  But this also needs to return
     * FALSE in order to get out of the loop, otherwise
     * it will stay there infinitely.
     *
     * The deferred retcode will accomplish this.
     */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, true);

    /*
     * Invoke again
     */
    SAMPLE_AppMain();

    /*
     * Confirm that CFE_SB_RcvMsg() (inside the loop) was called
     */
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_SB_RcvMsg)) == 1,
            "CFE_SB_RcvMsg() called");

    /*
     * Now also make the CFE_SB_RcvMsg call fail,
     * to exercise that error path.  This sends an
     * event which can be checked with a hook function.
     */
    UT_SetDeferredRetcode(UT_KEY(CFE_ES_RunLoop), 1, true);
    UT_SetDeferredRetcode(UT_KEY(CFE_SB_RcvMsg), 1, CFE_SB_PIPE_RD_ERR);
    UT_CheckEvent_Setup(&EventTest, SAMPLE_PIPE_ERR_EID, "SAMPLE APP: SB Pipe Read Error, App Will Exit");

    /*
     * Invoke again
     */
    SAMPLE_AppMain();

    /*
     * Confirm that the event was generated
     */
    UtAssert_True(EventTest.MatchCount == 1,
            "SAMPLE_PIPE_ERR_EID generated (%u)",
            (unsigned int)EventTest.MatchCount);
}

void Test_SAMPLE_AppInit(void)
{
    /*
     * Test Case For:
     * int32 SAMPLE_AppInit( void )
     */

    /* nominal case should return CFE_SUCCESS */
    UT_TEST_FUNCTION_RC(SAMPLE_AppInit(), CFE_SUCCESS);

    /* trigger a failure for each of the sub-calls,
     * and confirm a write to syslog for each.
     * Note that this count accumulates, because the status
     * is _not_ reset between these test cases. */
    UT_SetDeferredRetcode(UT_KEY(CFE_EVS_Register), 1, CFE_EVS_INVALID_PARAMETER);
    UT_TEST_FUNCTION_RC(SAMPLE_AppInit(), CFE_EVS_INVALID_PARAMETER);
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_ES_WriteToSysLog)) == 1,
            "CFE_ES_WriteToSysLog() called");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_CreatePipe), 1, CFE_SB_BAD_ARGUMENT);
    UT_TEST_FUNCTION_RC(SAMPLE_AppInit(), CFE_SB_BAD_ARGUMENT);
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_ES_WriteToSysLog)) == 2,
            "CFE_ES_WriteToSysLog() called");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 1, CFE_SB_BAD_ARGUMENT);
    UT_TEST_FUNCTION_RC(SAMPLE_AppInit(), CFE_SB_BAD_ARGUMENT);
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_ES_WriteToSysLog)) == 3,
            "CFE_ES_WriteToSysLog() called");

    UT_SetDeferredRetcode(UT_KEY(CFE_SB_Subscribe), 2, CFE_SB_BAD_ARGUMENT);
    UT_TEST_FUNCTION_RC(SAMPLE_AppInit(), CFE_SB_BAD_ARGUMENT);
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_ES_WriteToSysLog)) == 4,
            "CFE_ES_WriteToSysLog() called");

    UT_SetDeferredRetcode(UT_KEY(CFE_TBL_Register), 1, CFE_TBL_ERR_INVALID_OPTIONS);
    UT_TEST_FUNCTION_RC(SAMPLE_AppInit(), CFE_TBL_ERR_INVALID_OPTIONS);
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_ES_WriteToSysLog)) == 5,
            "CFE_ES_WriteToSysLog() called");

}

void Test_SAMPLE_ReportHousekeeping(void)
{
    /*
     * Test Case For:
     * void SAMPLE_ReportHousekeeping( const CFE_SB_CmdHdr_t *Msg )
     */
    SAMPLE_APP_SendHkCommand_t  CmdMsg;
    SAMPLE_APP_HkTlm_t          HkTelemetryMsg;

    memset(&CmdMsg, 0, sizeof(CmdMsg));
    memset(&HkTelemetryMsg, 0, sizeof(HkTelemetryMsg));

    /*
     * Force CmdCounter and ErrCounter to known values
     */
    SAMPLE_AppData.CmdCounter = 22;
    SAMPLE_AppData.ErrCounter = 11;

    /*
     * CFE_SB_InitMsg() needs to be done to set the emulated MsgId and Length.
     *
     * The FSW code only does this once during init and relies on it
     * remaining during the SAMPLE_ReportHousekeeping().  This does
     * not happen during UT so it must be initialized again here.
     */
    CFE_SB_InitMsg(&SAMPLE_AppData.HkBuf.MsgHdr,
                   SAMPLE_APP_HK_TLM_MID,
                   sizeof(SAMPLE_AppData.HkBuf),
                   true);

    /*
     * Set up to "capture" the telemetry message
     */
    UT_SetDataBuffer(UT_KEY(CFE_SB_SendMsg), &HkTelemetryMsg,
            sizeof(HkTelemetryMsg), false);

    SAMPLE_ReportHousekeeping(&CmdMsg);

    /*
     * check that the known values got into the telemetry correctly
     */
    UtAssert_True(HkTelemetryMsg.Payload.CommandCounter == 22,
            "HkTelemetryMsg.Payload.CommandCounter (%u) == 22",
            (unsigned int)HkTelemetryMsg.Payload.CommandCounter);

    UtAssert_True(HkTelemetryMsg.Payload.CommandErrorCounter == 11,
            "HkTelemetryMsg.Payload.CommandErrorCounter (%u) == 11",
            (unsigned int)HkTelemetryMsg.Payload.CommandErrorCounter);

    /*
     * Confirm that the CFE_TBL_Manage() call was done
     */
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_TBL_Manage)) == 1,
            "CFE_TBL_Manage() called");

}

void Test_SAMPLE_NoopCmd(void)
{
    /*
     * Test Case For:
     * void SAMPLE_NoopCmd( const SAMPLE_Noop_t *Msg )
     */
    SAMPLE_APP_Noop_t TestMsg;
    UT_CheckEvent_t EventTest;

    memset(&TestMsg, 0, sizeof(TestMsg));

    /* test dispatch of NOOP */
    UT_CheckEvent_Setup(&EventTest, SAMPLE_COMMANDNOP_INF_EID, NULL);

    UT_TEST_FUNCTION_RC(SAMPLE_Noop(&TestMsg), CFE_SUCCESS);

    /*
     * Confirm that the event was generated
     */
    UtAssert_True(EventTest.MatchCount == 1,
            "SAMPLE_COMMANDNOP_INF_EID generated (%u)",
            (unsigned int)EventTest.MatchCount);
}

void Test_SAMPLE_ResetCounters(void)
{
    /*
     * Test Case For:
     * void SAMPLE_ResetCounters( const SAMPLE_ResetCounters_t *Msg )
     */
    SAMPLE_APP_ResetCounters_t TestMsg;
    UT_CheckEvent_t EventTest;

    memset(&TestMsg, 0, sizeof(TestMsg));

    UT_CheckEvent_Setup(&EventTest, SAMPLE_COMMANDRST_INF_EID, "SAMPLE: RESET command");

    UT_TEST_FUNCTION_RC(SAMPLE_ResetCounters(&TestMsg), CFE_SUCCESS);

    /*
     * Confirm that the event was generated
     */
    UtAssert_True(EventTest.MatchCount == 1,
            "SAMPLE_COMMANDRST_INF_EID generated (%u)",
            (unsigned int)EventTest.MatchCount);
}

void Test_SAMPLE_ProcessCC(void)
{
    /*
     * Test Case For:
     * void  SAMPLE_ProcessCC( const SAMPLE_Process_t *Msg )
     */
    SAMPLE_APP_Process_t TestMsg;
    SAMPLE_APP_Table_t TestTblData;
    void *TblPtr = &TestTblData;

    memset(&TestTblData, 0, sizeof(TestTblData));
    memset(&TestMsg, 0, sizeof(TestMsg));

    /* Provide some table data for the SAMPLE_Process() function to use */
    TestTblData.Int1 = 40;
    TestTblData.Int2 = 50;
    UT_SetDataBuffer(UT_KEY(CFE_TBL_GetAddress), &TblPtr, sizeof(TblPtr), false);
    UT_TEST_FUNCTION_RC(SAMPLE_Process(&TestMsg), CFE_SUCCESS);

    /*
     * Confirm that the CFE_TBL_GetAddress() call was done
     */
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_TBL_GetAddress)) == 1,
            "CFE_TBL_GetAddress() called");


    /*
     * Confirm that the SAMPLE_Function() call was done
     * NOTE: This stub is provided by the sample_lib library
     */
    UtAssert_True(UT_GetStubCount(UT_KEY(SAMPLE_Function)) == 1,
            "SAMPLE_Function() called");

    /*
     * Configure the CFE_TBL_GetAddress function to return an error
     * Exercise the error return path
     */
    UT_SetForceFail(UT_KEY(CFE_TBL_GetAddress), CFE_TBL_ERR_UNREGISTERED);
    UT_TEST_FUNCTION_RC(SAMPLE_Process(&TestMsg), CFE_TBL_ERR_UNREGISTERED);
}

void Test_SAMPLE_TblValidationFunc(void)
{
    /*
     * Test Case For:
     * int32 SAMPLE_TblValidationFunc( void *TblData )
     */
    SAMPLE_APP_Table_t TestTblData;

    memset(&TestTblData, 0, sizeof(TestTblData));

    /* nominal case (0) should succeed */
    UT_TEST_FUNCTION_RC(SAMPLE_TblValidationFunc(&TestTblData), CFE_SUCCESS);

    /* error case should return SAMPLE_APP_TABLE_OUT_OF_RANGE_ERR_CODE */
    TestTblData.Int1 = 1 + SAMPLE_APP_TBL_ELEMENT_1_MAX;
    UT_TEST_FUNCTION_RC(SAMPLE_TblValidationFunc(&TestTblData),
            SAMPLE_APP_TABLE_OUT_OF_RANGE_ERR_CODE);
}




void Test_SAMPLE_GetCrc(void)
{
    /*
     * Test Case For:
     * void SAMPLE_GetCrc( const char *TableName )
     */

    /*
     * The only branch point here is CFE_TBL_GetInfo()
     *
     * Either way this function just does a write to syslog,
     * and it is the same in both cases, just with
     * a different message.  This could actually verify
     * the message using a hook function, if desired.
     */

    UT_SetForceFail(UT_KEY(CFE_TBL_GetInfo), CFE_TBL_ERR_INVALID_NAME);
    SAMPLE_GetCrc("UT");
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_ES_WriteToSysLog)) == 1,
            "CFE_ES_WriteToSysLog() called");

    UT_ClearForceFail(UT_KEY(CFE_TBL_GetInfo));
    SAMPLE_GetCrc("UT");
    UtAssert_True(UT_GetStubCount(UT_KEY(CFE_ES_WriteToSysLog)) == 2,
            "CFE_ES_WriteToSysLog() called");

}



/*
 * Setup function prior to every test
 */
void Sample_UT_Setup(void)
{
    UT_ResetState(0);
}

/*
 * Teardown function after every test
 */
void Sample_UT_TearDown(void)
{

}


/*
 * Register the test cases to execute with the unit test tool
 */
void UtTest_Setup(void)
{
    ADD_TEST(SAMPLE_AppMain);
    ADD_TEST(SAMPLE_AppInit);
    ADD_TEST(SAMPLE_ReportHousekeeping);
    ADD_TEST(SAMPLE_NoopCmd);
    ADD_TEST(SAMPLE_ResetCounters);
    ADD_TEST(SAMPLE_ProcessCC);
    ADD_TEST(SAMPLE_TblValidationFunc);
    ADD_TEST(SAMPLE_GetCrc);
}





