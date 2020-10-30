# vita-launcher

Support Forum
https://gbatemp.net/threads/release-vita-launcher-v1-6-app-to-launch-games-and-apps.574644/

This is a app/game launcher application.

The main purpose of this app is for users who have hundreds of games on their vita.

The psvita has limitation of 500 bubbles only. I couldn't find an app that served me well
to manage my library of games so I decided to create my own.

Use in conjuction with the CopyIcons app to get icons not created by psvita. https://github.com/cy33hc/copyicons

![Screenshot](screenshot.jpg)

## Features

1. Games are groups into multiple categories (vita/psp/homebrew/favorites) including retro consoles like nes,snes,gb,gbc,gba,n64 etc..
2. Can launch all vita games, Adernaline bubbles and (retro games without bubbles).
3. Has parental control for hiding certain settings.
4. Customize categories via configuration file.
3. Favorites. With hundreds of games, it's takes a quite a while to browse all the pages to find your favorite game. Therefore I've implemented a feature to add a game to you favorites for easy access.
4. Game images loaded on demand. With hundreds of games, we can't possibly load all the game images into memory. So images are loaded on demand only and a few pages of images are cached.
5. Last but not least, you can access you full library of games/apps. Just an FYI, I could load 1600 games/apps on my vita-tv.

## Controls

- up/down/left/right - for browsing your games 
- left analog stick - for browsing your games
- square - add the selected game to favorites
- circle - un-select or back.
- triangle - display settings to switch between Grid/List view.
- cross - start the selected game/app
- L-trigger - previous page
- R-trigger - next page

## How tos
1. How to change parental controls. https://youtu.be/bq6ZggMmocc
2. How to customize the category where bubble goes into. https://youtu.be/lNG8ox7cj4A
3. How to import roms into the launcher. https://youtu.be/hsADyvClBC4
4. How to change the retro core used to start a retro game. https://youtu.be/G49xqrjOcNY
5. How to customize icons for the rom that display in the app. https://youtu.be/TRvWKSKki3Q
6. How to setup application to Boot Adrenaline PSP ISO and EBOOT directly without bubbles. https://youtu.be/wkubbIsd8DY
   
## Build
To build this app, you will need to build the dependency imgui-vita2d (https://github.com/cy33hc/imgui-vita2d).
