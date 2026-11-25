    INCLUDE('UltimateDebug.inc'),ONCE
 
!--------------------------------------------------------------------------------
UltimateDebugProcedureTracker   CLASS,TYPE,MODULE('UltimateDebugProcedureTracker.CLW'),LINK('UltimateDebugProcedureTracker.CLW')   !,_ABCLinkMode_),DLL(_ABCDllMode_)
!
!Properties
!----------
!     
udReferenceObj                      &UltimateDebug  
TraceOn                             LONG
ProcedureID                         LONG

!Methods
!-------
Init                                PROCEDURE(UltimateDebug pUD,<STRING pProcedure>,<STRING pModuleName>,<STRING pAppName>,<STRING pModified>)

Destruct                            PROCEDURE()  

Trace                               Procedure(string pStr),Virtual
                                END


