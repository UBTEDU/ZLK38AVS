![Microsemi AVS banner](../../wiki/pictures/Microsemi_AVS_HmPgBnr_2017_06.jpg)
---
## About the project
Microsemi AcuEdge™ Development Kit for Amazon AVS is engineered to help you evaluate voice-enabled front-end audio systems for your Alexa-enabled products. This kit features Microsemi’s ZL38063 voice processor powered by Microsemi’s proprietary AcuEdge™ technology for front-end audio clean-up and Sensory’s TrulyHandsFree™ “Alexa” wake-word engine. A two microphone configuration allow you to test applications with 180° or 360° audio pick-up.

Please see [Howto](https://github.com/Microsemi/ZLK38AVS/wiki/howto) for quick start how-to instructions.



## What’s new?
**November 22, 2017:**
* V1.1.0: New Amazon Sample application

**May 30, 2017:**
* V1.0.0: Initial release



## Important considerations
* IMPORTANT: If you encounter the following compilation error with Raspbian Stretch:
```
/lib/modules/4.9.41-v7+/build: No such file or directory
```
   It means that the installer cannot find the headers matching the installed Linux kernel. One workaround is to update the kernel as follow:
```
sudo rpi-update a6b3e852ca70f2a12850b4542438583cc3b29788
```
* IMPORTANT: It is not recommended to use this demonstration platform in Headless Mode (see "Alexa sample app autoboot (headless mode)" during the installation) as it is always better to monitor the Amazon Sample App interface
* To update from the previous Java sample application, use the following commands (from the ZLK38AVS folder):
```
make cleanall
git pull
make all
```
* The Sensory wake word engine included with this project is time-limited: code linked against it will stop working when the library expires. The library included in this repository will, at all times, have an expiration date that is at least 120 days in the future. Use the following command to renew the license (from the ZLK38AVS folder):
```
make update_sensory
```
* Sometimes while playing music, the music will not stop when saying "Alexa". It is important to monitor the Amazon Sample App interface to see when "Alexa" is detected


## Resources
  * [ZLK38AVS Product Brief](https://github.com/Microsemi/ZLK38AVS/blob/master/docs/Microsemi_ZLK38AVS_ProductBrief.pdf) &#x1f517;
  * [ZLK38AVS Quick Start Guide](https://github.com/Microsemi/ZLK38AVS/blob/master/docs/Microsemi_ZLK38AVS_Quickstart.pdf) &#x1f517;
  * [ZLK38AVS User Guide](https://github.com/Microsemi/ZLK38AVS/blob/master/docs/Microsemi_ZLK38AVS_User_Guide.pdf) &#x1f517;
  * [ZLE38AVS Hardware User Guide](https://github.com/Microsemi/ZLK38AVS/blob/master/docs/Microsemi_ZLE38AVS_Hardware_User_Guide.pdf) &#x1f517;
