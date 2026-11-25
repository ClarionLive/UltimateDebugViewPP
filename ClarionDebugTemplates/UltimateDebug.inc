    COMPILE('****ASCII',_C60_)
                                PRAGMA ('link (C%V%ASC%X%%L%.LIB)')
    !END-COMPILE('****ASCII',_C60_)
    OMIT('****ASCII',_C60_)
    INCLUDE('_ver_.inc')  !sets _VER_ based on the CW version
LinkClass_Ascii                 CLASS,TYPE,LINK('C' & _VER_ & 'ASCX.LIB'); END  !Purpose is to link in the ASCII driver
    !END-OMIT('****ASCII',_C60_)

!
!------------------------------------
!ClarionLive Ultimate Debug Class
!Category: Clarion Live!
!
!Debug class to send messages to DebugView, available at http://technet.microsoft.com/en-us/sysinternals/bb896647  
!
!Code contributed by:
!     Mike Hanson
!     Mark Goldberg
!     John Hickey
!     Skip Williams see http://www.clarionmag.com/cmag/v5/v5n01debuger.html
!                                                                            
!Class Methods:
!
!     Debug(<string>) - Send any string to the debug window
!     DebugQueue(<queue>,<string>) - Display the contents of a queue
!     DebugEvent - Use to display events and additional useful information
!     DebugClear - Clears the DebugView window  
!
!Class Properties
!
!     DebugOff - TRUE to turn debugging off, FALSE to turn debugging on
!     DebugPrefix - Used to filter Debug messages, prepended to the Output String
!     SaveToFile - if TRUE, will save debug messages to an ASCII file                           
!     ASCIIFileName - name of ASCII file to save debug messages to, default is 'Debug.txt'
!------------------------------------------------------------------------------
udb_SettingsType                GROUP,TYPE
  
                                END

qProceduresQueueType            QUEUE,TYPE
AppName                             STRING(100)
ModuleName                          STRING(100)
ProcedureName                        STRING(100)
Modified                            STRING(26)      
Thread                              LONG  
ID                                  LONG
Count                               LONG

                                END     

qThreadCounterQueueType         QUEUE,TYPE
Thread                              LONG
Count                               LONG
                                END


qCurrentProcedureOnThreadQueueType      QUEUE,TYPE
Thread                                      LONG
ID                                          LONG
                                        END

ST::DebugEventQueue             QUEUE,TYPE
Date                                STRING(10)
Time                                STRING(10)
EventNo                             SIGNED
EventName                           CSTRING(31)
FieldFeq                            SIGNED
FieldName                           CSTRING(31)
Keycode                             UNSIGNED
StarDate                            REAL
                                END

DebugCategoryQueue              QUEUE,TYPE
Category                            STRING(40)
                                END

TemporaryPrefixQueueType        QUEUE,TYPE
Prefix                              STRING(100)
                                END
   
ExceptionQType                  QUEUE, TYPE
ExceptName                          STRING(100)
                                END


GlobalASCIIFileName             STRING('DebugLog_yyyymmdd_hhmmss.txt')

!If you want a prefix to be included in the beginning of each debug string,
!then copy this to your application directory, and edit it as desired.
!!ST::DEBUG:Prefix                EQUATE('!')
!!ST::DEBUG:Debugging             EQUATE(1)  

qtUserEventName                 QUEUE,TYPE
EventEquate                         LONG
EventName                           CSTRING(40) !~Mar/19/08 MG
                                END 

KeyStateUD:Shift                EQUATE(0100h) !Shift key is pressed
KeyStateUD:Ctrl                 EQUATE(0200h) !Ctrl key is pressed
KeyStateUD:Alt                  EQUATE(0400h) !Alt key is pressed
KeyStateUD:EnterOnNum           EQUATE(0800h) !Num Pad Enter Key, n/a with MESSAGE()
KeyStateUD:CapsLock             EQUATE(1000h) !Caps Lock is On
KeyStateUD:NumLock              EQUATE(2000h) !Num Lock is On
KeyStateUD:ScrollLock           EQUATE(4000h) !Scroll Lock is On
KeyStateUD:InsertKey            EQUATE(8000h) !Insert Mode is On,  
eqDBG                           EQUATE('<4,2,7>') ! CTRL+D, CTRL+B, CTRL+G   !added 5/22/03 by Mark Goldberg (to be used by Assert(x,y) as in Assert(0,eqDBG&'my message')
  
log:ProceduresOnly              EQUATE(1)
log:AllDebugMessages            EQUATE(2)



!--------------------------------------------------------------------------------
UltimateDebug                   CLASS,TYPE,MODULE('UltimateDebug.CLW'),LINK('UltimateDebug.CLW')  !,_ABCLinkMode_),DLL(_ABCDllMode_)
!
!--------------------------------------------------------------------------------
! Add properties here, which are variables
!--------------------------------------------------------------------------------
!Properties
!----------
DebugOff                            BYTE(FALSE)  
DebugPrefix                         STRING(60)
SaveToFile                          BYTE(FALSE) 
ASCIIFileName                       STRING(FILE:MaxFilePath+FILE:MaxFileName)   
SaveToJson                          BYTE(FALSE)
JSONFileName                        STRING(FILE:MaxFilePath+FILE:MaxFileName)
ProcedureInfoToDebugView            BYTE(FALSE)  
UsePlainFormat                      BYTE(FALSE)  
MessagePrefix                       STRING(1000)

     
DebugNoCR                           BYTE(FALSE)
LineWrap                            BYTE(TRUE) 

qProcedures                         &qProceduresQueueType 
qThreadCounter                      &qThreadCounterQueueType 
qCurrentProcedureOnThread           &qCurrentProcedureOnThreadQueueType 
ExceptionQ                          &ExceptionQType
                                                
ModuleName                          STRING(100)
AppName                             STRING(100)
Modified                            STRING(26)

udb_Settings                        GROUP(udb_SettingsType)
                                    END

ProcedureName                       CSTRING(100)
ProcedureThread                     LONG
       
ProcedureID                         LONG


DebugEvent                          SIGNED
HotKey                              UNSIGNED
PurgeStarTime                       REAL
EventQ                              &ST::DebugEventQueue
IgnoreEventQ                        &ST::DebugEventQueue

TemporaryPrefixQueue                &TemporaryPrefixQueueType

CategoryQueue                       &DebugCategoryQueue
CriticalSection                     &ICriticalSection
                                            
EventOffset                         LONG      
UserEventNameQ                      &qtUserEventName  

ShowAll                             Bool
ShowField                           Bool
ShowFocus                           Bool
ShowSelected                        Bool
ShowSelStart                        Bool
ShowSelEnd                          Bool
ShowKeyCode                         Bool
ShowError                           Bool
ShowThread                          Bool
ShowContents                        Bool
ShowScreenText                      Bool
ShowAcceptAll                       Bool  

ProcAlertKey                        LONG
TraceOn                             LONG

!--------------------------------------------------------------------------------
! Add Methods here, which are just procedures or functions
!--------------------------------------------------------------------------------
!Methods
!-------

Init                                PROCEDURE(<STRING pProcedure>,<STRING pModuleName>,<STRING pAppName>,<STRING pModified>),LONG,PROC
EndOfProcedure                      PROCEDURE(LONG pID)
AddCategoryToDebug                  PROCEDURE(STRING pCategory)
AddUserEvent                        PROCEDURE(STRING argEventName,Long argEventEquate)
CalcStarDate                        PROCEDURE(<LONG D>,<LONG T>),REAL,PRIVATE
Construct                           PROCEDURE()   
!region Debug Comment
!!! <summary>
!!!Send to Debug viewer
!!!</summary>   
!!! <param name="STRING pCustomPrefix">Custom prefix header, used for filtering</param>
!!! <param name="STRING">STRING pDebugString</param>
!!! <param name="STRING">Optional STRING pCustomPrefix</param>
!!! <param name="STRING">STRING pDebugString</param>
!!! <param name="STRING">Optional STRING pCustomPrefix</param>
!!! <param name="STRING">STRING pDebugString</param>       
!endregion
Debug                               PROCEDURE(STRING pDebugString,<STRING pCustomPrefix>,BYTE pNoClip = 0,BYTE pForce = 0,<STRING pID>) 
ClearDebugView                      PROCEDURE() !<summary>This method displays a debug window for a queue<br/></summary><remarks>This method displays a debug window for a queue, which includes a list of the fields in the queue and the records in the queue. It also has the option to send the queue to the debug view.<br/><br/></remarks><remarks>  This method displays a debug window for a queue, which includes a list of the fields in the queue and the records in the queue. It also has the option to send the queue to the debug view.<br/> <br/>Parameters:<br/>    --   *QUEUE pQueue: pointer to the queue to be debugged<br/>--   [STRING pMsg]: optional message to be displayed in the debug window<br/>--   [BYTE pNoTouch]: optional flag to indicate if the queue pointer should be restored<br/>--   BYTE pSendToDebugView=0: flag to indicate if the queue should be sent to the debug view<br/>--   LONG pMaximumRecords=30: maximum number of records to be sent to the debug view<br/> <br/>Properties used<br/>--  SELF.DebugOff<br/> <br/>Methods called:<br/>--  SELF.FormatMessageList(pMsg, MsgLineQ)<br/>--  SELF.Debug(usl.Get())<br/></remarks> 
DebugEvent                          PROCEDURE(<STRING pDebugProcedure>)  
GPF                                 PROCEDURE()
DebugGroup                          PROCEDURE(*GROUP pGroup,<STRING pMsg>,<STRING pStructureType>,BYTE pSendToDebugView=0)
!DebugGroup                          PROCEDURE(*GROUP pGroup,<STRING pMsg>,<STRING pStructureType>)       
!region Comment: DebugQueue      
!!!Summary:                                                   
!!! UltimateDebug.DebugQueue is a procedure that processes a passed queue for debugging purposes. It checks if 
!!! the queue is NULL, and if it's not, it scans it and loads its fields into another queue. It then prepares a 
!!! window to display information about the queue. The method also provides options for displaying formatted 
!!! message lists, sending debug data to a debug view, or displaying a window with several user-interactive 
!!! buttons for operations like copying a row or sending a row to debug. 
!!!
!!!Parameters:
!!! *QUEUE pQueue: This is a pointer to the queue that is passed to the method. It is used for the debugging 
!!! process.
!!! <STRING pMsg>: This parameter represents a string message that can be displayed. It is optional.
!!! <BYTE pNoTouch>: This is a flag that indicates whether to restore the queue pointer after the debugging 
!!! process. It is optional.
!!! BYTE pSendToDebugView=0: This is a flag that controls whether the debug data should be sent to a debug view.
!!! It is optional and its default value is 0.
!!! LONG pMaximumRecords=30: This parameter indicates the maximum number of records to be displayed. It is 
!!! optional and its default value is 30.
!!!
!!!Remarks:
!!! UltimateDebug.DebugQueue procedure begins by checking if the debugging is off. If it isn't, it saves the 
!!! current queue pointer, scans the passed queue, and loads its fields into another queue. It then prepares a 
!!! window to display information about the queue. If the `pMsg` parameter is provided, it formats the message 
!!! list. Otherwise, it hides the message list. If `pSendToDebugView` is set, it sends the debug data to a debug 
!!! view. If not, it displays a window with various buttons for user interaction. Finally, it restores the queue 
!!! pointer, if `pNoTouch` is not set to 1.
!!!
!!!Returns: 
!!! Nothing is returned.
!!!
!!!Properties used:
!!! SELF.DebugOff: This property indicates whether debugging is turned off.
!!! SELF.FormatMessageList: This method formats the message list.
!!! SELF.Debug: This method sends the debug information.
!!!
!!!Methods called:
!!! SELF.DebugOff(): Checks if the debugging is turned off.
!!! SELF.FormatMessageList(pMsg, MsgLineQ): Formats the message list with the provided message and queue.
!!! SELF.Debug(usl.Get()): Sends the debug information.
!endregion
DebugQueue                          PROCEDURE(*QUEUE pQueue,<STRING pMsg>,<BYTE pNoTouch>,BYTE pSendToDebugView=0,LONG pMaximumRecords=30)
DebugRecord                         PROCEDURE(*FILE pFile,<STRING pMsg>)
!!DebugRecord                         PROCEDURE(*FILE pFile1,*FILE pFile2,<STRING pMsg>)   
DecToHex                            PROCEDURE(LONG pNumber),STRING
Destruct                            PROCEDURE()
FormatMessageList                   PROCEDURE(STRING pMsg,*QUEUE pMsgQueue)  
GetEventDescr                       PROCEDURE(LONG   argEvent=-1),STRING,PRIVATE !defaults to EVENT()
GetEventName                        PROCEDURE(SIGNED Event),STRING,PRIVATE
GetFEQDescr                         PROCEDURE(SIGNED argFEQ=-1  ),STRING,PRIVATE !defaults to FIELD()
GetUserEvent                        PROCEDURE(LONG argEventEquate),STRING,PRIVATE   
HuntForOffset                       PROCEDURE(),private
IgnoreEvent                         PROCEDURE(SIGNED Event)
LogEvent                            PROCEDURE   
Message                             PROCEDURE(STRING pDebugString)           
!region Comment: SendOutputToDebug                   
!!!Summary:                                                   
!!!  The `UltimateDebug.SendOutputToDebug` method accepts a string and potentially a custom prefix, and determines whether to clip the string. This information is then processed for debugging output.
!!!
!!!Parameters:
!!!   STRING pOutput - The main string input for the method. If this is an empty string, the method will return without performing any actions. There is no default value.
!!!   <STRING pCustomPrefix> - An optional parameter that provides a custom prefix for the output. This parameter does not have a default value.
!!!   BYTE pNoClip = 0 - This parameter determines whether the output string will be clipped. The default value is 0.
!!!
!!!Remarks:
!!!  The method prepares a string for debugging output. If the `pOutput` string is empty, the method will return immediately. If not, the string will be prepared for output, potentially including clipping and the addition of a custom prefix. The actual output is handled by the `SendLineToDebug()` procedure.
!!!
!!!Returns: 
!!!  Nothing is returned.
!!!
!!!Properties used
!!!  No SELF. or PARENT. properties are explicitly used in the provided method.
!!!
!!!Methods called:
!!!  SendLineToDebug() - This procedure is called within the method to handle the actual output to the debug stream.
!endregion
SendOutputToDebug                   PROCEDURE(STRING pOutput,<STRING pCustomPrefix>,BYTE pNoClip = 0,<STRING pID>)
SetApplicationName                  PROCEDURE(STRING pApplicationName,STRING pProgramExtension),STRING
SetShortApplicationName             PROCEDURE(STRING pApplicationName,STRING pProgramExtension),STRING
SetDebugEvent                       PROCEDURE(SIGNED DebugEvent)
SetEventOffSet                      PROCEDURE(),private
SetHotKey                           PROCEDURE(UNSIGNED HotKey)
SetPurgeTime                        PROCEDURE(LONG DebugPurge)
ShowProcedureInfo                   PROCEDURE(STRING pProcedure,STRING pApplication,STRING pHelpID,STRING pCreated,STRING pModified,STRING pCompiled)
TakeEvent                           PROCEDURE()

TurnOnTracing                       PROCEDURE(FILE pFile)
TurnoffTracing                      PROCEDURE(FILE pFile)
GetFieldNames                       PROCEDURE(FILE pFile)

SetPrefix                           PROCEDURE(STRING pPrefix)
BeginTemporaryPrefix                PROCEDURE(STRING pPrefix)
EndTemporaryPrefix                  PROCEDURE()
                   
AddProcedureException               PROCEDURE(STRING pProcedureName)
RemoveProcedureException            PROCEDURE(STRING pProcedureName)
EmptyProcedureExceptions            PROCEDURE()  

Error                               PROCEDURE(<STRING pMessage>),BYTE,PROC  
ErrorCode                           PROCEDURE(<STRING pMessage>),BYTE,PROC  

NewFilteredView                     PROCEDURE()

InitAssertHook                      PROCEDURE
                         
Wait                                PROCEDURE(Long pId),Virtual
Release                             PROCEDURE(Long pId),Virtual
Trace                               PROCEDURE(string pStr),Virtual 

EscapeString                        PROCEDURE(STRING value),STRING

Enable                              PROCEDURE()
Disable                             PROCEDURE()

                                END 

