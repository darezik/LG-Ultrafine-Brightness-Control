# LG-Ultrafine-Brightness-Control

A lightweight Windows app to adjust LG Ultrafine 4K/5K brightness.

This program allows users to:
- View the current brightness level of the monitor.
- Adjust brightness using a slider.
- Control brightness via keyboard (left and right arrow keys).

This project is derived from the CLI tool [LG-Ultrafine-Brightness](https://github.com/csujedihy/LG-Ultrafine-Brightness). Special thanks to [@csujedihy](https://github.com/csujedihy). 

## Features

- **Real-time Brightness Adjustment**: Adjust the brightness using an interactive slider or keyboard keys.
- **Dynamic Connection Support**: Handles monitor connections and disconnections gracefully.
- **Lightweight Interface**: Minimal UI design for a focused and intuitive user experience.

## Build Instructions

1. Open the `.sln` file in Visual Studio.
2. Select the `x64` build configuration.
3. Build the project.
4. Run the resulting executable.

## Usage

1. **Run the Program**:
   - Launch the application executable.
2. **Adjust Brightness**:
   - Use the slider with your mouse to change brightness.
   - Alternatively, use the left and right arrow keys to change brightness.

## Code Overview

### Key Features in Code

- **Device Communication**:
  - Uses `hidapi` for communicating with the LG Ultrafine monitor.
  - `get_brightness()` and `set_brightness()` functions retrieve and update the brightness values, respectively.
  
- **UI Implementation**:
  - Built with Win32 API, featuring a slider and a label to display brightness in percentage.
  - Supports both mouse and keyboard interaction for brightness control.

- **Dynamic Updates**:
  - Automatically detects connected LG Ultrafine 4K/5K monitors.
  - Updates UI elements in real-time based on user/device interactions.

### Dependencies

- [hidapi](http://github.com/signal11/hidapi): For communicating with HID devices. 

### License

This project is licensed under the GNU General Public License v3.0.

You are free to:

Use, modify, and distribute this project under the terms of the GPL v3 license. However, this program is provided "as is," without any warranty of any kind, express or implied, including but not limited to the warranties of merchantability or fitness for a particular purpose. See the GNU General Public License for more details.

For the full license text, refer to the LICENSE file included in this repository or visit [GNU GPL v3 License](https://www.gnu.org/licenses/gpl-3.0.en.html).
