# Using Codice-markers

## **What is a Codice Marker?**

Codice marker is a 2D barcode, similar to QR-code. Codice is a proprietary format that MultiTaction displays detect and track along with fingers and IR-pens. Codice markers have few different formats (3x3, 4x4, 5x5). Selection between the different dimensions can be done as a tradeoff between different unique ID’s and the print size of the marker. F.ex 3x3 is physically smaller than the 4x4 marker but it has less unique codes. 

Dimension is configured in the MultiTaction display OSD. Default value is 4x4.

## **Creating the Markers**

Markers can be generated as .png images using an utility tool MarkerFactory that ships with Cornerstone SDK ([https://cornerstone.multitouch.fi/](https://cornerstone.multitouch.fi/)).  More info about usage can be found from this link:[https://cornerstone.multitouch.fi/taction-guide/creatingmarkers.html](https://cornerstone.multitouch.fi/taction-guide/creatingmarkers.html)

Table 1: Main differences between 3x3 and 4x4 markers

| Dimension | Number of Unique Codes | Minimum Print Size | Recommended Print Size |
| :---- | :---- | :---- | :---- |
| 3x3 | 32 | 35mm | 45mm |
| 4x4 | 4096 | 40mm | 50mm |

   
Notice that the size of the marker **includes the black border around the white rectangle.** Making the physical size of the marker bigger will make the tracking more reliable and is thus recommended.

It is also recommended that the material where the marker is attached (thus facing the display) is entirely black. This way it affects the tracking less and should not produce any spurious finger-touches. The other side of the marker can be freely used for branding (company logo’s) or f.ex business cards, etc.

| Notice\! In order for it to work the marker needs to be placed against the MultiTaction display glass. The marker must not be wrinkled or even partly separated from the glass surface. |
| :---- |

Image 1: Marker dimensions

## **Material**

Good quality material  improves the tracking robustness and increases the lifetime of the markers. We recommend polyester-based  **Xerox Never Tear** material which should be easily available from most professional print houses and it can also be ordered for example from Amazon.

We recommend always using a professional print house for the actual printing. However you can make perfectly fine markers using normal print paper and laser printer. Here are however few drawbacks for such approach:

* Paper and the print will wear off quite quickly when in active use. In some cases you might have to replace the markers only after a few hours of active use.   
* The reliability of the tracking may not be optimal because of the poor contrast, depending on paper and print quality.

## **Tracking quality** 

The quality and robustness of the tracking can be increased by 

* Increasing the physical size of the marker  
* Increasing the size of the black border  
* Using high-quality material