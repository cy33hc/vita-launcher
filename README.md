# vita-launcher

This is a app/game launcher application.

The main purpose of this app is for users who have hundreds of games on their vita.

The psvita has limitation of 500 bubbles only. I couldn't find an app that served me well
to manage my library of games so I decided to create my own. 

## Features

1. Games are display in 4 categories (vita/psp/honebrew/favorites). The psp game categories looks for games starting the the gameid PSPEMU. Mainly the bubbles created with Adrenaline Bubble Manager (https://github.com/ONElua/AdrenalineBubbleManager)
2. Game caching. With hundreds of games, scanning the folders and building the list takes a long time. So a cache is built on the first scan and  subsequent startup of the app reads from the cache.
3. Favorites. With hundreds of games, it's takes a quite a while to browse all the pages to find your favorite game. Therefore I've implemented a feature to add a game to you favorites for easy access.
4. Game images loaded on demand. With hundreds of games, we can't possibly load all the game images into memory. So images are loaded on demand only and a few pages of images are cached.
5. Last but not least, you can access you full library of games/apps. Just an FYI, I could load 1600 games/apps on my vita-tv.

## Controls

up/down/left/right - for browsing your games 
left analog stick - for browsing your games
square - add the selected game to favorites
circle - switch game/app category
triangle - scan games and rebuild the cache.
cross - start the selected game/app
L-trigger - previous page
R-trigger - next page

## Build
To build this app, you will need to build the dependency imgui-vita2d (https://github.com/cy33hc/imgui-vita2d).