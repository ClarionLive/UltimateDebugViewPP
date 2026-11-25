                                MEMBER()
    omit('***',_c55_)
_ABCDllMode_                    EQUATE(0)
_ABCLinkMode_                   EQUATE(1)
    ***
!
!--------------------------
!ClarionLive! Basic Class Template
!--------------------------

    INCLUDE('EQUATES.CLW')
    INCLUDE('UltimateDebugProcedureTracker.INC'),ONCE

                                MAP
                                END
 

!---------------------------------------
UltimateDebugProcedureTracker.Destruct  PROCEDURE()
!---------------------------------------
    CODE
    
    IF ~SELF.udReferenceObj &= NULL
        SELF.udReferenceObj.EndOfProcedure(SELF.ProcedureID)  
    END    


!-----------------------------------
UltimateDebugProcedureTracker.Init      PROCEDURE(UltimateDebug pUD,<STRING pProcedure>,<STRING pModuleName>,<STRING pAppName>,<STRING pModified>)
!-----------------------------------

    CODE      
    
    SELF.udReferenceObj  &=  pUD 
    SELF.ProcedureID = SELF.udReferenceObj.Init(pProcedure,pModuleName,pAppName,pModified)

                                                                                              
!------------------------------------------------------------------------------
UltimateDebugProcedureTracker.Trace     PROCEDURE(string pStr)
!------------------------------------------------------------------------------

szMsg                                       CSTRING(len(pStr)+10)

    CODE
    
    IF SELF.TraceOn
        szMsg  =  '[un] ' & CLIP(pStr)
!!        unm_ud.DebugPrefix =  'UNM'
!!        unm_ud.DebugOff    =  FALSE
!!        unm_ud.Debug(szMsg)
    END  

