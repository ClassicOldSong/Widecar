# Widecar

Widecar is a tiny helper designed to enhance your streaming experience with Sunshine by automatically managing virtual screens. This app seamlessly integrates with ParsecVDD to create and destroy virtual screens based on your connection status, and it ensures your virtual display matches the Moonlight device resolution settings.

`Widecar` is basically `Sidecar for Windows`

## Features

1. **Automatically create a new virtual screen on connect**: When you start streaming, Widecar will automatically create a new virtual display.
2. **Automatically destroy the virtual screen on exit**: Upon exiting, Widecar will remove the virtual display to keep your system clean.
3. **Automatically match the Moonlight device resolution settings**: Widecar adjusts the virtual display settings to match your Moonlight device resolution for a smooth streaming experience.
4. **Easy to use**: Simply execute the app upon connection and exit. See the screenshot below for configuration.
5. **Multi-instances support**: Widecar supports multiple Sunshine instances running simultaneously for multiple sidecar screen configuration.

## Requirements

### Sunshine Streaming
- [Sunshine Streaming](https://github.com/LizardByte/Sunshine)
  - **Important**: Leave the `Output Name` configuration blank, or the newly added virtual display won't be picked up.

### ParsecVDD
- [ParsecVDD](https://github.com/nomi-san/parsec-vdd)
  - The ParsecVDA driver must be installed.
  - Install first, run it, then put it in the background. You don't need to do anything more.
  - Other persistent services that poll the vidsplay server constantly, such as [ParsecVDA-Always-Connected](https://github.com/timminator/ParsecVDA-Always-Connected), are also compatible.

## Configuration

To use Widecar, simply configure it to run upon connection and exit in your desired app. Elevated must be checked. Below is an example configuration screenshot:

![Configuration Screenshot](https://github.com/ClassicOldSong/Widecar/assets/10512422/20331aa5-9372-43f3-b79c-4e84a61e843d)

## License

This project is licensed under the MIT License.
