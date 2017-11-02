![Microsemi AVS banner](../../wiki/pictures/Microsemi_AVS_HmPgBnr_2017_06.jpg)
---
## About the project
Microsemi AcuEdge™ Development Kit for Amazon AVS is engineered to help you evaluate voice-enabled front-end audio systems for your Alexa-enabled products. This kit features Microsemi’s ZL38063 voice processor powered by Microsemi’s proprietary AcuEdge™ technology for front-end audio clean-up and Sensory’s TrulyHandsFree™ “Alexa” wake-word engine. A two microphone configuration allow you to test applications with 180° or 360° audio pick-up.

Please see [Howto](https://github.com/Microsemi/ZLK38AVS/wiki/howto) for quick start how-to instructions.



## What’s new?
**May 30, 2017:**
* V1.0.0: Initial release

## Important considerations
* IMPORTANT: If you encounter the following compilation error with Raspbian Stretch:
```
/lib/modules/4.9.41-v7+/build: No such file or directory
```

   It means that the installer cannot find the headers matching the installed Linux kernel. One workarround is to update the kernel as follow:
```
sudo rpi-update a6b3e852ca70f2a12850b4542438583cc3b29788
```
* IMPORTANT: The Sensory wake word engine included with this project is time-limited: code linked against it will stop working when the library expires. The library included in this repository will, at all times, have an expiration date that is at least 120 days in the future. See [Sensory's GitHub page](https://github.com/Sensory/alexa-rpi) for more information on how to renew the license for non-commercial use.
* Due to some limitation in the Alexa Reference implementation, volume 5 should be considered the maximum volume.

## Resources
  * [ZLK38AVS Product Brief](https://github.com/Microsemi/ZLK38AVS/blob/master/docs/Microsemi_ZLK38AVS_ProductBrief.pdf) &#x1f517;
  * [ZLK38AVS Quick Start Guide](https://github.com/Microsemi/ZLK38AVS/blob/master/docs/Microsemi_ZLK38AVS_Quickstart.pdf) &#x1f517;
  * [ZLK38AVS User Guide](https://github.com/Microsemi/ZLK38AVS/blob/master/docs/Microsemi_ZLK38AVS_User_Guide.pdf) &#x1f517;
  * [ZLE38AVS Hardware User Guide](https://github.com/Microsemi/ZLK38AVS/blob/master/docs/Microsemi_ZLE38AVS_Hardware_User_Guide.pdf) &#x1f517;
