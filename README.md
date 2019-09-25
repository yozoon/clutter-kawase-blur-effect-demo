#  Clutter Dual Kawase Blur Demo
GTK app demonstrating a new Clutter effect based on the dual Kawase blur method. 
This project aims to implement the fast blur effect found in the KDE KWin window manager with tools native to the GNOME Desktop environment.

![Window Screenshot](https://raw.githubusercontent.com/yozoon/clutter-kawase-blur-effect-demo/master/screenshot.png "Window Screenshot")

## Compilation
```bash
git clone https://github.com/yozoon/clutter-kawase-blur-effect-demo.git
cd clutter-kawase-blur-effect-demo
meson <builddir>
cd <builddir>
ninja
```
This generates an executable called "blur_demo" inside the <builddir>.

## Roadmap
| Task | Status |
|:----|:----|
| Create GTK app that contains a GTK-Clutter embed | Done |
| Figure out how to compile a standalone clutter effect | Done |
| Replace box blur shader with a single downsample pass | Done |
| Figure out how to use the CoglPipeline and CoglSnippet APIs | Done |
| Use Meson build system | Done |
| Figure out how to chain CoglPipelines | Done |
| Implement blur with hardcoded values | Done |
| Implement the blur strength calculation function | Done |
| Create a function which allows setting the blur strength | Done |
| Use this function in combination with a GTK Scale to set the desired blur strength | Done |
| Find a way to properly benchmark the performance of the effect | Planned |
| Make the effect animatable by using the Clutter animation framework | Planned |
| Tweak the offset and iteration values to make the transitions smoother | Optional |
| ... | ... |

## Contribute
I'm sure there are still lots of memory optimisations and type checks that can be implemented to further improve the code quality. Also I tried to adhere to the coding style I found in the Clutter project, but I might have overlooked some inconsistencies - so feel free to create a new PR ;)

## Credits
Credits go to Emmanuele Bassi who implemented the original box blur clutter effect as well as Alex Nemeth and the team at KWin for the work they put into implementing the blur effect on which this Clutter effect is based.
Links:
* [clutter-blur-effect.c](https://gitlab.freedesktop.org/pq/mutter/blob/master/clutter/clutter/clutter-blur-effect.c)
* [alex47 blur effect demo app](https://github.com/alex47/Dual-Kawase-Blur)
* [KWin Repository (blur effect subfolder)](https://phabricator.kde.org/source/kwin/browse/master/effects/blur/)