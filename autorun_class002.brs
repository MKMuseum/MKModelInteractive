

'
' BrightScript that recieves keybord input and displays images according to keystroke.
' Key press keys and image files are coordinated via media list provide in a configuration file called  
' <mediaInfo.json>. 
'
' Ver 0.1 Date 30/05/23 Author Mark R Dornan
'

Sub Main()
    '
    ' Instantiate a keyboard message manager object
    '
    kbMsgMngr =  newKeyboardMessageMngr()
    '
    ' load the media list into the manager
    '
    kbMsgMngr.LoadMediaList("mediaInfo.json")
    '
    ' show the default image
    '
    kbMsgMngr.ShowImage(kbMsgMngr.mediaList.Lookup("Z"))
    '
    ' Invoke the keyboard manager's event loop
    '
    While true
        kbMsgMngr.EventLoop()
        if kbMsgMngr.Exit() then ExitWhile
    End While

End Sub

'
' BrightScript constructor.   newKeyboardMessageMngr() is regular Function of module scope
'
Function newKeyboardMessageMngr() As Object
    
    'Instantiate container for Message Manager member data and member functions 
    msgManager=CreateObject("roAssociativeArray")  
    
    'Member Data
   
    ' Instantiate AA with for media info from configuration file 
    msgManager.mediaList = CreateObject("roAssociativeArray")
   
    ' Instantiate a message port to recieve keyboard events
    msgManager.port = CreateObject("roMessagePort")                                                                            
    msgManager.keyboard = CreateObject("roKeyboard")
    msgManager.keyboard.SetPort(msgManager.port) ' instruct keyboard to send events to port

    ' Instantiate an image player to display static media
    msgManager.rectangle = CreateObject("roRectangle", 0, 0, 1920, 1080)
    msgManager.imagePlayer = CreateObject("roImagePlayer")
    msgManager.imagePlayer.SetRectangle(msgManager.rectangle)
    
    'Member Functions
    msgManager.LoadMediaList = mmLoadMediaList
    msgManager.ShowImage = mmShowImage 
    msgManager.EventLoop = mmEventLoop
    msgManager.Exit = mmExit

    return msgManager

End Function


Function mmLoadMediaList(jsonFileName$ as String) as Void
    
    fileStream = CreateObject("roReadFile", jsonFileName$)
    response = ParseJson(fileStream.ReadBlock(65536))
    
    For Each medium In response.media
        m.mediaList.AddReplace(medium.key,medium.file_path)
    End For

End Function

Function mmShowImage(fileName$ as String) As Void
    
    m.imagePlayer.DisplayFile(fileName$)
    m.imagePlayer.Show()

End Function


Function mmEventLoop() As Void
   
    while true
        msg = wait(10, m.port) ' wait for a message
        if type(msg) = "roKeyboardPress" then
            ' Save the keypress to a variable                          
            key$ = chr(msg.GetInt())
            ' process the keypress
            if  m.mediaList.DoesExist(key$) then
                m.ShowImage(m.mediaList.Lookup(key$))
            endif
        else
            ' ignore other non=keyboard events
        endif
    end while 

End Function


Function mmExit() as Boolean 
    
    return false
    
End Function


