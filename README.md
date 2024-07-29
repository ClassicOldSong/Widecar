# Widecar

Widecar is a tiny helper designed to enhance your streaming experience with Sunshine by automatically managing virtual screens. This app seamlessly integrates with ParsecVDD to create and destroy virtual screens based on your connection status, and it ensures your virtual display matches the Moonlight device resolution settings.

`Widecar` is basically `Sidecar for Windows`, it is especially useful when you have an Android tablet with high refresh rate and a stylus. If you connect a keyboard and mouse to the tablet, it's no different from a Windows tablet but with much powerful CPU/GPU power when streaming.

## Features

1. **Automatically create a new virtual screen on connect**: When you start streaming, Widecar will automatically create a new virtual display.
2. **Automatically destroy the virtual screen on exit**: Upon exiting, Widecar will remove the virtual display to keep your system clean.
3. **Automatically match the Moonlight device resolution settings**: Widecar adjusts the virtual display settings to match your Moonlight device resolution for a smooth streaming experience.
4. **Easy to use**: Simply execute the app upon connection and exit. See the screenshot below for configuration.
5. **Multi-instances support**: Widecar supports multiple Sunshine instances running simultaneously for multiple sidecar screen configuration.

Also checkout https://github.com/ClassicOldSong/moonlight-android/releases for a much improved Moonlight Android client with much better touch pad support and several other improvements, since the official one hasn't been updated for months...

## Requirements

### Sunshine Streaming
- [Sunshine Streaming](https://github.com/LizardByte/Sunshine)
  - **Important**: Leave the `Output Name` configuration blank, or the newly added virtual display won't be picked up.

### ParsecVDD
- [ParsecVDD](https://github.com/nomi-san/parsec-vdd)
  - The ParsecVDA driver must be installed.
  - Install first, **run it**, then put it in the background. You don't need to do anything more.
  - Read [here](https://github.com/nomi-san/parsec-vdd#design-notes) for explaination of why a daemon is needed:
    > You have to ping to the driver periodically to keep added displays alive, otherwise all of them will be unplugged after a second.

## Configuration

To use Widecar, simply configure it to run upon connection and exit in your desired app. Elevated must be checked. Below is an example configuration screenshot:

![Configuration Screenshot](https://github.com/ClassicOldSong/Widecar/assets/10512422/20331aa5-9372-43f3-b79c-4e84a61e843d)

You can find a pre-built version in the [release page](https://github.com/ClassicOldSong/Widecar/releases).

## Advanced usage

You can copy the `sunshine.conf` in your Sunshine installation, name it like `sunshine_widecar.conf`, and start a second Sunshine instance with it by executing `sunshine.exe path\to\sunshine_widecar.conf` with SYSTEM privilege.

The [start-widecar.ps1](start-widecar.ps1) should help you with that.

You can then create a shortcut to that script, setting target to `powershell -ExecutionPolicy Bypass -File "path\to\start-widecar.ps1"`

You need to execute the script with Administrator privilege. Starting the instance with only Administrator privilege is fine, but you won't be able to access UAC dialog or enter lock screen password through streaming.

Add/change the following parts to `sunshine_widecar.conf` in order to run parallel with the original instance:

```ini
# Change the port according to your situation
port = 48989
# You can change the name as you wish
sunshine_name = Widecar
# A separate state file must be created in order to distinguish between the original Sunshine instance
file_state = sunshine_state_widecar.json
# Change to a different log path to prevent conflicts
log_path = sunshine_widecar.log
# Change the path to where you actually put the widecar.exe
global_prep_cmd = [{"do":"D:\\Tools\\widecar.exe","undo":"D:\\Tools\\widecar.exe","elevated":"true"}]
```

Then you need to ensure `Global Prep Commands` is enabled for all of your apps. In this configuration, the `Widecar` instance share all apps from the main instance, but all apps started here are in a separated virtual screen.

You can start even more Sunshine instances by doing the above steps multiple times, then you'll be able to connect multiple Moonlight clients at the same time for even more virtual mointors.

## Troubleshooting

- **No virtual display added**
  - Ensure the ParsecVDA driver is installed
  - Ensure the ParsecVDD application or similar services that constantly poll the driver is running
- **Shows the same screen as main screen**
  - If you're using an external display for the first time, Windows might configure it as "Mirror mode" by default. Press <kbd>Meta + P</kbd> (or known as <kbd>Win + P</kbd>) and select "Extended", then **exit the app** (not only the stream) and start the app again. You only need to do this once.
- **Primary display changed to the virtual display after connection. I don't want that.**
  - Setting the virtual display to primary **before** the stream starts is a must for Sunshine to use the virtual screen for streaming.
    You can set the primary display back **after** the stream starts manually or using MultiMonitorTool to automate the process.

    https://www.nirsoft.net/utils/multi_monitor_tool.html

    Add to the **`Detatched Command`** like this with the proper display name to your "Desktop" app:

    ![image](https://github.com/user-attachments/assets/a29fca73-ca52-4651-9cdf-87dd6bf71da4)

    You can change the timeout accordingly. If changing the primary display back to the built-in display too quickly, Sunshine might still capture the built-in display.
- **Resolution can't match client side anymore**
  - ***NEVER*** set screen rotation on virtual displays! Widecar can handle vertical display normally, there's no need to manually set screen rotatition if you're using [Moonlight Noir](https://github.com/ClassicOldSong/moonlight-android) with Widecar.
  - If you happen messed up with your monitor config:
    1. Disconnect ALL Moonlight sessions
    2. Remove ALL virtual displays using ParsecVDD, or simply quit ParsecVDD or whatever service you're using to keep the driver alive
    3. <kbd>Meta(Win) + R</kbd>, then type `regedit`, hit enter
    4. Delete ALL entries under
        - `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers\Configuration`
        - `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers\Connectivity`
        - `HKEY_LOCAL_MACHINE\SOFTWARE\Widecar\sessions`

    This will clear your monitor configuration cache and Widecar connection cache.

    Then you're good to go!
- **Stream failed to start with 2 or more graphics card**
  - When your system has more than 2 graphic cards, ParsecVDA might got confused with which card to use as backend. 

    Follow the `Select Parent GPU` section of [this instruction](https://support.parsec.app/hc/en-us/articles/4423615425293-VDD-Advanced-Configuration) to manually set your desired GPU to use with the virtual display.

    > Visit the following Registry path on your system and add the specified key and value.
    >
    >```
    >HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\WUDF\Services\ParsecVDA\Parameters
    >```
    >
    >Insert new `DWORD` with name `PreferredRenderAdapterVendorId`, then depending on your system hardware enter the following Vendor ID as hexadecimal in the Registry key value field.
    >
    >| Vendor |  ID  |
    >|--------|------|
    >| NVIDIA | 10DE |
    >| AMD    | 1002 |
    >
    >To apply this change to the Parsec Virtual Display Adapter, either reboot the host or disable and enable the Virtual Display Adapter inside device manager.

    You might also want to set `DisablePreferredRenderAdapterChange` to `0` if it exists in the registry path. [[Source](https://github.com/ClassicOldSong/Widecar/issues/2#issuecomment-2254636625)]

    If you have done the above steps and still got no image, try set `Adapter Name` in your Sunshine's `Audio/Video` config to the same video card's name of which you're using with the VDA driver.

## Disclaimer

I got kicked from Moonlight and Sunshine's Discord server and banned from Sunshine's GitHub repo literally for helping people out.

![image](https://github.com/user-attachments/assets/f01fc57f-5199-4495-9b96-68cfa017b7ff)

This is what I got for finding a bug, opened an issue, getting no response, troubleshoot myself, fixed the issue myself, shared it by PR to the main repo hoping my efforts can help someone else during the maintainance gap.

Yes, I'm going away. I have started [a fork of Sunshine called Apollo](https://github.com/ClassicOldSong/Apollo) and will add useful features that will never get merged by the main repo shortly. [Apollo](https://github.com/ClassicOldSong/Apollo) and [Moonlight Noir](https://github.com/ClassicOldSong/moonlight-android) will no longer be compatible with OG Sunshine and OG Moonlight eventually, but they'll work even better with much more carefully designed features.

The Moonlight repo had stayed silent for 5 months, with nobody actually responding to issues, and people are getting totally no help besides the limited FAQ in their Discord server. I tried to answer issues and questions, solve problems within my ablilty but I got kicked out just for helping others. The funniest thing is, the repo starts updating after they got me banned!

**PRs for feature improvements are welcomed here unlike the main repo, your ideas are more likely to be appreciated and your efforts are actually being respected. We welcome people who can and willing to share their efforts, helping yourselves and other people in need.**


## License

This project is licensed under the MIT License.
