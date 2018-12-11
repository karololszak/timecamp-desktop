#!/usr/bin/env bash

# script that fixes weird macOS 10.14 SDK issues
# SO: https://stackoverflow.com/questions/51314888/ld-warning-text-based-stub-file-are-out-of-sync-falling-back-to-library-file#comment93288538_53111739
# script source: https://gist.github.com/wawiesel/eba461de5f5e38f7f0ac93ae3676b484
# slightly modified to include PrivateFrameworks too

F=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks
G=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/PrivateFrameworks

for d in $(ls -d $F/*.framework);
do
sudo ln -s $d /Library/Frameworks/;
done

for d in $(ls -d $F/ApplicationServices.framework/Versions/A/Frameworks/*.framework);
do
sudo ln -s $d /Library/Frameworks/;
done

for d in $(ls -d $F/CoreServices.framework/Versions/A/Frameworks/*.framework);
do
sudo ln -s $d /Library/Frameworks/;
done


for d in $(ls -d $F/Carbon.framework/Versions/A/Frameworks/*.framework);
do
sudo ln -s $d /Library/Frameworks/;
done

for d in $(ls -d $G/*.framework);
do
sudo ln -s $d /Library/Frameworks/;
done
