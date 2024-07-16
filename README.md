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
  - Setting the virtual display to primary before the stream starts is a must for sunshine to use the virtual screen for streaming.
    You can set the primary display back after the stream starts manually or using MultiMonitorTool to automate the process.

    https://www.nirsoft.net/utils/multi_monitor_tool.html

    Add to the **`game command`** like this with the proper display name to your "Desktop" app:

    ![image](https://github.com/user-attachments/assets/3ed9a9ca-857a-4871-b235-43f802a8f60f)

    You can obtain the display name by running the MultiMonitorTool directly from Explorer.

## License

This project is licensed under the MIT License.
