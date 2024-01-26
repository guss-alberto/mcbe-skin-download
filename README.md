# MCBE-skin-download
Download Minecraft Bedrock Editions skins to use on Java Edition

A simple proof-of-concpet script that read raw images from the memory allocated to Minecraft and tries to match the alpha transparency byte pattern of 64x64 dual layer minecraft skins (the same used by java) in order to allow players of both version to use their purchased Bedrock skins on Java.

The images are saved in an 16kB uncompressed PNG file that is somehwat non compliant but works on most software (apparently not FireFox) anyways.

## Usage: 
just compile and run the program on Windows with Minecraft: Bedrock Edition running, it will simply find the minecraft process, read the memory and save all the skins found in a /skins/ folder.

## Known issues:
 - Sometimes textures other than skins might be downloaded
 - For some reason the PNG files created don't render on Firefox (no, it's not the checksum idk why)


# Disclamer: Only use this to save skins you have purchased, I do not encourage Piracy.
