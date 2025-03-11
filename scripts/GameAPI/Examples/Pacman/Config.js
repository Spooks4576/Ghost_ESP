// Configuration settings
const config = {
    asset_folder: "./assets",
    output: "pacman.espg",
    compress_logic: false
};

// Asset definitions
const assets = [
    { name: "pacman.png", type: "sprite", format: "rgb565" },
{ name: "ghost.png", type: "sprite", format: "rgb565" },
{ name: "maze.png", type: "image", format: "indexed8" }
];

// Initial positions (in pixels)
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
    // Move ghost (simplified static movement for now; Math.random() not supported yet)
    moveSprite("ghost", ghostX + 1, ghostY);

    // Animate Pac-Man
    setAnimation("pacman", pacmanAnim);
    pacmanAnim = (pacmanAnim === "open") ? "close" : "open";
    moveSprite("pacman", pacmanX, pacmanY);

    // Draw a power pellet
    drawPixel(pacmanX + 10, pacmanY, YELLOW);

    // Draw touch control zones (for visualization, static for 128x128)
    drawRect(96, 96, 20, 20, BLUE, false); // Right
    drawRect(32, 96, 20, 20, BLUE, false); // Left
    drawRect(64, 32, 20, 20, BLUE, false); // Up
    drawRect(64, 96, 20, 20, BLUE, false); // Down
}

// Touch controls with relative coordinates (rel_x, rel_y, rel_radius, callback)
onTouchPress(0.85, 0.85, 0.1, function() {
    pacmanX = pacmanX + 5; // Dynamic assignment
    drawLine(pacmanX - 5, pacmanY, pacmanX, pacmanY, YELLOW);
});

onTouchPress(0.15, 0.85, 0.1, function() {
    pacmanX = pacmanX - 5;
    drawLine(pacmanX + 5, pacmanY, pacmanX, pacmanY, YELLOW);
});

onTouchPress(0.5, 0.15, 0.1, function() {
    pacmanY = pacmanY - 5;
    drawLine(pacmanX, pacmanY + 5, pacmanX, pacmanY, YELLOW);
});

onTouchPress(0.5, 0.85, 0.1, function() {
    pacmanY = pacmanY + 5;
    drawLine(pacmanX, pacmanY - 5, pacmanX, pacmanY, YELLOW);
});
