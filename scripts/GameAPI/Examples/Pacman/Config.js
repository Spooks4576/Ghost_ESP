// Configuration settings
const config = {
    asset_folder: "./assets",
    output: "pacman.espg",
    compress_logic: false
};


const assets = [
    { name: "pacman.png", type: "sprite", format: "rgb565" },  // Index 0
{ name: "ghost.png", type: "sprite", format: "rgb565" },   // Index 1
{ name: "maze.png", type: "image", format: "indexed8" }    // Index 2
];

// Asset indices
const PACMAN = 0;
const GHOST = 1;
const MAZE = 2;


let pacmanX = 50;
let pacmanY = 50;
let ghostX = 80;
let ghostY = 50;
let pacmanAnim = "open";

// Colors (RGB565)
const YELLOW = 0xFFE0;
const RED = 0xF800;
const BLUE = 0x001F;

// Main game loop
function update() {
    ghostX = ghostX + 1;  // Update ghostX dynamically
    moveSprite(GHOST, ghostX, ghostY);

    // Animate Pac-Man
    setAnimation(PACMAN, pacmanAnim);
    pacmanAnim = (pacmanAnim === "open") ? "close" : "open";
    moveSprite(PACMAN, pacmanX, pacmanY);

    // Draw a power pellet
    drawPixel(pacmanX + 10, pacmanY, YELLOW);


    drawRect(96, 96, 20, 20, BLUE, false); // Right
    drawRect(32, 96, 20, 20, BLUE, false); // Left
    drawRect(64, 32, 20, 20, BLUE, false); // Up
    drawRect(64, 96, 20, 20, BLUE, false); // Down
}


onTouchPress(0.85, 0.85, 0.1, function() {
    pacmanX = pacmanX + 5; // Move right
    drawLine(pacmanX - 5, pacmanY, pacmanX, pacmanY, YELLOW);
});

onTouchPress(0.15, 0.85, 0.1, function() {
    pacmanX = pacmanX - 5; // Move left
    drawLine(pacmanX + 5, pacmanY, pacmanX, pacmanY, YELLOW);
});

onTouchPress(0.5, 0.15, 0.1, function() {
    pacmanY = pacmanY - 5; // Move up
    drawLine(pacmanX, pacmanY + 5, pacmanX, pacmanY, YELLOW);
});

onTouchPress(0.5, 0.85, 0.1, function() {
    pacmanY = pacmanY + 5; // Move down
    drawLine(pacmanX, pacmanY - 5, pacmanX, pacmanY, YELLOW);
});
