IDK what you put in these...

build requirements i guess:
 - you need SFML 2.5 or later
 - probably some other stuff
 - build with: "./build.sh"

how to run:
 - for the server, run: "./Server"
 - for the client, run: "./Client", and input the server ip address

controls:
 - WASD to move
 - x button to close

conf files:
    server:
`
listen port
max client connections at one time
`

    client:
`
server address
port
default color (r g b)
default spell 1
default spell 2
default spell 3
`