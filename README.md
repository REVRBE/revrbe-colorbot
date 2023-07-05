# revrbe-colorbot

A simple colorbot with trigger and aimbot functionality using BitBlt to capture the colors. 

## Usage

To record colors; simply press INSERT once to start the recording and hold mouse5 to begin recording the colors that are located at the cursors position.   
To exclude colors; simply hold HOME while pressing mouse5 to exclude colors at the cursors position.   

Here is a video, where I showcase it: 

[colorbot video](https://streamable.com/gn5vzg)

## NB!

Keep in mind that this code has not been tested on any anti-cheats other than VAC - there is a risk to get banned from using this. 

Also, it has false-positives where it detect a color which is near/very close to the recorded colors - the threshold setting can help bring down false-positives.  
But I recommend maybe doing a bigger rectangle horizontally and analyzing the detected colors and check how many pixels the colors stretch across - this way it could bring down false-positives as it would just ignore those pixels where the color-area is very small. Anyway, have fun with the repository :)
