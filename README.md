# GoldenEye: Source
## About
A total conversion modification of Half-Life 2. We are a fan made artistic
recreation, released for free, with a primary goal in mind;  To bring the
memories and experiences from the original GoldenEye-64 back to life using
the Source SDK 2007 Technology. As one of the premier and consistently released
HL2 mods, GES has been seen out and about, reviewed by the best, and played by
hundreds and thousands of PC gamers for Eight long years and running.

Visit our [website](http://www.geshl2.com) and [forums](http://forums.geshl2.com)!

## Current State
GE:S is under active development.  Though most aspects of the development proccess happen on the GE:S Team's private servers, the base code of the game is open source and provided here.

## Navigating the Code
The main code base for GES is located in the following directories:

- /game/ges

## Third-Party Libraries
GES relies on several third-party libraries during compilation. We have an integrated
Python 2.6 interpreter to run our Gameplay Scenarios and AI. To drive this, we use
the Boost Python library. To play our music, we utilize FMOD since Valve's internal
sound functions do not operate across level transitions.

## Contributing
If you have a bug to fix or feature to contribute, please give us a bug report or pull request!  All we ask is for clear documentation of the change and for your understanding if we decide not to include it.  Minor bug fixes are more lilely to be accepted than major feature additions, but if there's a feature contribution you'd like to work on feel free to ask ahead of time if it's something we're interested in including in the game.

## Code License
GoldenEye: Source original code (files starting with 'ge_') is licensed under the
GNU GPL v3. The full license text can be found in LICENSE_GES.

It's important to know that this code is supplied WITH ABSOLUTELY NO WARRANTY WHATSOEVER.
We are not responsible for any damages resulting from the use of this code. Please see the
license for more information.
