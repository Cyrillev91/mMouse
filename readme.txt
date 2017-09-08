This mMouse v0.2d is brought to you by ceezblog.info - 2015

This program will fix the three-finger-tap of ASUS Smart Gesture that open Cortana into middle mouse button.
For windows 10 users who miss the middle mouse function which likely support by all other laptops but ASUS.
Work on most ASUS laptops run windows 10 that doesn't allow to to change the option of three-finger gesture. 

Version 0.2a support 3 finger swipe for backward and forward button

How to install?
- Copy mMouse.exe to %AppData%\Microsoft\Windows\Start Menu\Programs\Startup
- Then restart.

Or simply run it each time computer start


History: 

Version 0.2b:
- Add 3 finger swipe fix -> backward / forward button

Version 0.1
- Fix 3 finger tap -> middle mouse button

Version 1.0:
- Recode from scratch



------------------------------------------------------------------------------------------------------------

Modified by Cyrillev91 :

09/07/2016 :
- Open File Explorer With 3 fingers up gesture
- Middle click now with 2 fingers Tap and Right click with 3 fingers Tap
23/07/2016 :
- Add Debug string info
- optimize 3 fingers Tap and optimze option to disable 3/2 fingers Tap
http://dl.free.fr/v3CtrC74N
https://github.com/Cyrillev91/mMouse/releases/latest


01/09/2017 :
Fix : send Middle and Right Click
MSDN : https://msdn.microsoft.com/en-us/library/windows/desktop/ms644986(v=vs.85).aspx
The hook procedure should process a message in less time than the data entry specified in the LowLevelHooksTimeout value in the following registry key:
HKEY_CURRENT_USER\Control Panel\Desktop
The value is in milliseconds. If the hook procedure times out, the system passes the message to the next hook.  <---
However, on Windows 7 and later, the hook is silently removed without being called.
There is no way for the application to know whether the hook is removed.
==> Use thread to send Middle and Right Click so that the function is faster than LowLevelHooksTimeout
    (and change LowLevelHooksTimeout in Windows 10 has no effect ???)
==> Send Open Windows Explorer by Thread

08/09/2017
Use Thread to send Backward / Forward (3 fingers swipe left / right)
Fix : Send RButtonDown in some case
Fix : SendKey() function
Disable OutputDebugStringA() function

Test : OK with SmartGesture 4.0.9 and 4.0.17
SmartGesture_Win10_64_VER409 http://dlcdnet.asus.com/pub/ASUS/nb/Apps_for_Win10/SmartGesture/SmartGesture_Win10_64_VER409.zip?_ga=2.172942123.962806994.1504290823-185335011.1500703387
