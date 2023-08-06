// Included libs
#include "Enemy.h"
#include "Particle.h"
#include "Spawner.h"
#include "Lava.h"
#include "Boss.h"
#include "Conveyor.h"

#define TIMEOUT             30000
#define LEVEL_COUNT         9
#define DIRECTION           1
#define MIN_WAIT_INTERVAL  16    // Min redraw interval (ms) 33 = 30fps / 16 = 63fps

// WOBBLE ATTACK
#define ATTACK_WIDTH        70     // Width of the wobble attack, world is 1000 wide
#define ATTACK_DURATION     500    // Duration of a wobble attack (ms)
#define BOSS_WIDTH          40
#define ATTACK_THRESHOLD    30000  // The threshold that triggers an attack
#define JOYSTICK_DEADZONE   5      // Angle to ignore

// PLAYER
#define MAX_PLAYER_SPEED    10     // Max move speed of the player

enum WebController { Idle, Forward, Backward, Shoot };

class TwangGame {
    private:
        long previousMillis = 0;           // Time of the last redraw
        int levelNumber = 0;
        long lastInputTime = 0;

        int brightness = 0;
        unsigned long mm = 0;
        
        // WOBBLE ATTACK
        long attackMillis = 0;             // Time the attack started
        bool attacking = 0;
        int joystickTilt = 0;              // Stores the angle of the joystick
        int joystickWobble = 0;            // Stores the max amount of acceleration (wobble)

        // PLAYER
        char* stage;                       // what stage the game is at (PLAY/DEAD/WIN/GAMEOVER)
        long stageStartTime;               // Stores the time the stage changed for stages that are time based
        int playerPosition;                // Stores the player position
        int playerPositionModifier;        // +/- adjustment to player position
        bool playerAlive;
        long killTime;
        int lives = 3;

        // POOLS
        int lifeLEDs[3] = {52, 50, 40};
        Enemy enemyPool[10] = {
            Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy()
        };
        int const enemyCount = 10;
        Particle particlePool[40] = {
            Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle()
        };
        int const particleCount = 40;
        Spawner spawnPool[2] = {
            Spawner(), Spawner()
        };
        int const spawnCount = 2;
        Lava lavaPool[4] = {
            Lava(), Lava(), Lava(), Lava()
        };
        int const lavaCount = 4;
        Conveyor conveyorPool[2] = {
            Conveyor(), Conveyor()
        };
        int const conveyorCount = 2;
        Boss boss = Boss();


        // ---------------------------------
        // ------------ LEVELS -------------
        // ---------------------------------
        void loadLevel(){
            updateLives();
            cleanupLevel();
            playerPosition = 0;
            playerAlive = 1;
            switch(levelNumber){
                case 0:
                    // Left or right?
                    playerPosition = 200;
                    spawnEnemy(1, 0, 0, 0);
                    break;
                case 1:
                    // Slow moving enemy
                    spawnEnemy(900, 0, 1, 0);
                    break;
                case 2:
                    // Spawning enemies at exit every 2 seconds
                    spawnPool[0].Spawn(1000, 3000, 2, 0, 0);
                    break;
                case 3:
                    // Lava intro
                    spawnLava(400, 490, 2000, 2000, 0, "OFF");
                    spawnPool[0].Spawn(1000, 5500, 3, 0, 0);
                    break;
                case 4:
                    // Sin enemy
                    spawnEnemy(700, 1, 7, 275);
                    spawnEnemy(500, 1, 5, 250);
                    break;
                case 5:
                    // Conveyor
                    spawnConveyor(100, 600, -1);
                    spawnEnemy(800, 0, 0, 0);
                    break;
                case 6:
                    // Conveyor of enemies
                    spawnConveyor(50, 1000, 1);
                    spawnEnemy(300, 0, 0, 0);
                    spawnEnemy(400, 0, 0, 0);
                    spawnEnemy(500, 0, 0, 0);
                    spawnEnemy(600, 0, 0, 0);
                    spawnEnemy(700, 0, 0, 0);
                    spawnEnemy(800, 0, 0, 0);
                    spawnEnemy(900, 0, 0, 0);
                    break;
                case 7:
                    // Lava run
                    spawnLava(195, 300, 2000, 2000, 0, "OFF");
                    spawnLava(350, 455, 2000, 2000, 0, "OFF");
                    spawnLava(510, 610, 2000, 2000, 0, "OFF");
                    spawnLava(660, 760, 2000, 2000, 0, "OFF");
                    spawnPool[0].Spawn(1000, 3800, 4, 0, 0);
                    break;
                case 8:
                    // Sin enemy #2
                    spawnEnemy(700, 1, 7, 275);
                    spawnEnemy(500, 1, 5, 250);
                    spawnPool[0].Spawn(1000, 5500, 4, 0, 3000);
                    spawnPool[1].Spawn(0, 5500, 5, 1, 10000);
                    spawnConveyor(100, 900, -1);
                    break;
                case 9:
                    // Boss
                    spawnBoss();
                    break;
            }
            stageStartTime = millis();
            stage = "PLAY";
        }

        void spawnBoss(){
            boss.Spawn();
            moveBoss();
        }

        void moveBoss(){
            int spawnSpeed = 2500;
            if(boss._lives == 2) spawnSpeed = 2000;
            if(boss._lives == 1) spawnSpeed = 1500;
            spawnPool[0].Spawn(boss._pos, spawnSpeed, 3, 0, 0);
            spawnPool[1].Spawn(boss._pos, spawnSpeed, 3, 1, 0);
        }

        void spawnEnemy(int pos, int dir, int sp, int wobble){
            for(int e = 0; e<enemyCount; e++){
                if(!enemyPool[e].Alive()){
                    enemyPool[e].Spawn(pos, dir, sp, wobble);
                    enemyPool[e].playerSide = pos > playerPosition?1:-1;
                    return;
                }
            }
        }

        void spawnLava(int left, int right, int ontime, int offtime, int offset, char* state){
            for(int i = 0; i<lavaCount; i++){
                if(!lavaPool[i].Alive()){
                    lavaPool[i].Spawn(left, right, ontime, offtime, offset, state);
                    return;
                }
            }
        }

        void spawnConveyor(int startPoint, int endPoint, int dir){
            for(int i = 0; i<conveyorCount; i++){
                if(!conveyorPool[i]._alive){
                    conveyorPool[i].Spawn(startPoint, endPoint, dir);
                    return;
                }
            }
        }

        void cleanupLevel(){
            for(int i = 0; i<enemyCount; i++){
                enemyPool[i].Kill();
            }
            for(int i = 0; i<particleCount; i++){
                particlePool[i].Kill();
            }
            for(int i = 0; i<spawnCount; i++){
                spawnPool[i].Kill();
            }
            for(int i = 0; i<lavaCount; i++){
                lavaPool[i].Kill();
            }
            for(int i = 0; i<conveyorCount; i++){
                conveyorPool[i].Kill();
            }
            boss.Kill();
        }

        void levelComplete(){
            stageStartTime = millis();
            stage = "WIN";
            if(levelNumber == LEVEL_COUNT) stage = "COMPLETE";
            lives = 3;
            updateLives();
        }

        void nextLevel(){
            levelNumber ++;
            if(levelNumber > LEVEL_COUNT) levelNumber = 0;
            loadLevel();
        }

        void gameOver(){
            levelNumber = 0;
            loadLevel();
        }

        void die(){
            playerAlive = 0;
            if(levelNumber > 0) lives --;
            updateLives();
            if(lives == 0){
                levelNumber = 0;
                lives = 3;
            }
            for(int p = 0; p < particleCount; p++){
                particlePool[p].Spawn(playerPosition);
            }
            stageStartTime = millis();
            stage = "DEAD";
            killTime = millis();
        }

        // ----------------------------------
        // -------- TICKS & RENDERS ---------
        // ----------------------------------
        void tickEnemies(){
            for(int i = 0; i<enemyCount; i++){
                if(enemyPool[i].Alive()){
                    enemyPool[i].Tick();
                    // Hit attack?
                    if(attacking){
                        if(enemyPool[i]._pos > playerPosition-(ATTACK_WIDTH/2) && enemyPool[i]._pos < playerPosition+(ATTACK_WIDTH/2)){
                           enemyPool[i].Kill();
                        }
                    }
                    if(inLava(enemyPool[i]._pos)){
                        enemyPool[i].Kill();
                    }
                    
                    // Hit player?
                    if(
                        (enemyPool[i].playerSide == 1 && enemyPool[i]._pos <= playerPosition) ||
                        (enemyPool[i].playerSide == -1 && enemyPool[i]._pos >= playerPosition)
                    ){
                        die();
                        return;
                    }
                }
            }
        }

        void drawEnemies(){
            for(int i = 0; i<enemyCount; i++){
                if(enemyPool[i].Alive()) {
                    strip.setPixelColor(getLED(enemyPool[i]._pos), CRGB(255, 0, 0));
                }
            }
        }

        void tickBoss(){
            // DRAW
            if(boss.Alive()){
                boss._ticks ++;
                for(int i = getLED(boss._pos-BOSS_WIDTH/2); i<=getLED(boss._pos+BOSS_WIDTH/2); i++){
                    strip.setPixelColor(i, CRGB::DarkRed);
                    strip.setPixelColor(i,100);
                }
                // CHECK COLLISION
                if(getLED(playerPosition) > getLED(boss._pos - BOSS_WIDTH/2) && getLED(playerPosition) < getLED(boss._pos + BOSS_WIDTH)){
                    die();
                    return; 
                }
                // CHECK FOR ATTACK
                if(attacking){
                    if(
                      (getLED(playerPosition+(ATTACK_WIDTH/2)) >= getLED(boss._pos - BOSS_WIDTH/2) && getLED(playerPosition+(ATTACK_WIDTH/2)) <= getLED(boss._pos + BOSS_WIDTH/2)) ||
                      (getLED(playerPosition-(ATTACK_WIDTH/2)) <= getLED(boss._pos + BOSS_WIDTH/2) && getLED(playerPosition-(ATTACK_WIDTH/2)) >= getLED(boss._pos - BOSS_WIDTH/2))
                    ){
                       boss.Hit();
                       if(boss.Alive()){
                           moveBoss();
                       }else{
                           spawnPool[0].Kill();
                           spawnPool[1].Kill();
                       }
                    }
                }
            }
        }

        void drawPlayer(){
            strip.setPixelColor(getLED(playerPosition), CRGB(0, 255, 0));
        }

        void drawExit(){
            if(!boss.Alive()){
                strip.setPixelColor(strip.getLength()-1, CRGB(0, 0, 255));
            }
        }

        void tickSpawners(){
            long mm = millis();
            for(int s = 0; s<spawnCount; s++){
                if(spawnPool[s].Alive() && spawnPool[s]._activate < mm){
                    if(spawnPool[s]._lastSpawned + spawnPool[s]._rate < mm || spawnPool[s]._lastSpawned == 0){
                        spawnEnemy(spawnPool[s]._pos, spawnPool[s]._dir, spawnPool[s]._sp, 0);
                        spawnPool[s]._lastSpawned = mm;
                    }
                }
            }
        }

        bool tickParticles(){
            bool stillActive = false;
            for(int p = 0; p < particleCount; p++){
                if(particlePool[p].Alive()){
                    particlePool[p].Tick(false);
                    strip.setPixelColor(getLED(particlePool[p]._pos), CRGB(particlePool[p]._power, 0, 0));
                    stillActive = true;
                }
            }
            return stillActive;
        }

        void tickConveyors(){
            int b, dir, n, i, ss, ee, led;
            long m = 10000+millis();
            playerPositionModifier = 0;

            for(i = 0; i<conveyorCount; i++){
                if(conveyorPool[i]._alive){
                    dir = conveyorPool[i]._dir;
                    ss = getLED(conveyorPool[i]._startPoint);
                    ee = getLED(conveyorPool[i]._endPoint);
                    for(led = ss; led<ee; led++){
                        b = 5;
                        n = (-led + (m/100)) % 5;
                        if(dir == -1) n = (led + (m/100)) % 5;
                        b = (5-n)/2.0;
                        if(b > 0) strip.setPixelColor(led, CRGB(0, 0, b));
                    }

                    if(playerPosition > conveyorPool[i]._startPoint && playerPosition < conveyorPool[i]._endPoint){
                        if(dir == -1){
                            playerPositionModifier = -(MAX_PLAYER_SPEED-4);
                        }else{
                            playerPositionModifier = (MAX_PLAYER_SPEED-4);
                        }
                    }
                }
            }
        }

        void drawAttack(){
            if(!attacking) return;
            int n = map(millis() - attackMillis, 0, ATTACK_DURATION, 100, 5);
            for(int i = getLED(playerPosition-(ATTACK_WIDTH/2))+1; i<=getLED(playerPosition+(ATTACK_WIDTH/2))-1; i++){
                strip.setPixelColor(i, CRGB(0, 0, n));
            }
            if(n > 90) {
                n = 255;
                strip.setPixelColor(getLED(playerPosition), CRGB(255, 255, 255));
            }else{
                n = 0;
                strip.setPixelColor(getLED(playerPosition), CRGB(0, 255, 0));
            }
            strip.setPixelColor(getLED(playerPosition-(ATTACK_WIDTH/2)), CRGB(n, n, 255));
            strip.setPixelColor(getLED(playerPosition+(ATTACK_WIDTH/2)), CRGB(n, n, 255));
        }

        int getLED(int pos){
            // The world is 1000 pixels wide, this converts world units into an LED number
            return constrain((int)map(pos, 0, 1000, 0, strip.getLength()-1), 0, strip.getLength()-1);
        }

        bool inLava(int pos){
            // Returns if the player is in active lava
            int i;
            Lava LP;
            for(i = 0; i<lavaCount; i++){
                LP = lavaPool[i];
                if(LP.Alive() && LP._state == "ON"){
                    if(LP._left < pos && LP._right > pos) return true;
                }
            }
            return false;
        }

        void updateLives(){
            // Updates the life LEDs to show how many lives the player has left
            for(int i = 0; i<3; i++){
               digitalWrite(lifeLEDs[i], lives>i?HIGH:LOW);
            }
        }



        // ---------------------------------
        // ----------- JOYSTICK ------------
        // ---------------------------------
        void getInput(){
            joystickWobble = 0;
            joystickTilt = 0;
            switch (webController)
            {
            case Shoot:
                joystickWobble = ATTACK_THRESHOLD + 1;
            
            case Forward:
                joystickTilt = 90;
                break;
            
            case Backward:
                joystickTilt = -90;
                break;
            }
            webController = Idle;
        }
    
    public:
        WebController webController = Idle;

        void setup() {
            // Life LEDs
            for(int i = 0; i<3; i++){
                pinMode(lifeLEDs[i], OUTPUT);
                digitalWrite(lifeLEDs[i], HIGH);
            }
        }

        char* getStage() {
            return stage;
        }

        int getLevel() {
            return levelNumber;
        }

        void render() {
            if(stage == "PLAY"){
                drawPlayer();
                drawAttack();
                drawEnemies();
                drawExit();
            }else if(stage == "WIN"){
                // LEVEL COMPLETE
                strip.fill(CRGB::Black);
                if(stageStartTime+500 > mm){
                    int n = max(map(((mm-stageStartTime)), 0, 500, strip.getLength(), 0), long(0));
                    for(int i = strip.getLength(); i>= n; i--){
                        brightness = 255;
                        strip.setPixelColor(i, CRGB(0, brightness, 0));
                    }
                }else if(stageStartTime+1000 > mm){
                    int n = max(map(((mm-stageStartTime)), 500, 1000, strip.getLength(), 0), long(0));
                    for(int i = 0; i< n; i++){
                        brightness = 255;
                        strip.setPixelColor(i, CRGB(0, brightness, 0));
                    }
                }else if(stageStartTime+1200 > mm){
                    strip.setPixelColor(0, CRGB(0, 255, 0));
                }
            }else if(stage == "COMPLETE"){
                strip.fill(CRGB::Black);
                if(stageStartTime+500 > mm){
                    int n = max(map(((mm-stageStartTime)), 0, 500, strip.getLength(), 0), long(0));
                    for(int i = strip.getLength(); i>= n; i--){
                        brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                        strip.setPixelColor(i, CHSV(brightness, 255, 50));
                    }
                }else if(stageStartTime+5000 > mm){
                    for(int i = strip.getLength(); i>= 0; i--){
                        brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                        strip.setPixelColor(i, CHSV(brightness, 255, 50));
                    }
                }else if(stageStartTime+5500 > mm){
                    int n = max(map(((mm-stageStartTime)), 5000, 5500, strip.getLength(), 0), long(0));
                    for(int i = 0; i< n; i++){
                        brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                        strip.setPixelColor(i, CHSV(brightness, 255, 50));
                    }
                }
            }
        }

        void tick() {
            mm = millis();
            brightness = 0;

            if (mm - previousMillis < MIN_WAIT_INTERVAL) {
                return;
            }

            getInput();
            previousMillis = mm;
            if(abs(joystickTilt) > JOYSTICK_DEADZONE){
                lastInputTime = mm;
                if(stage == "SCREENSAVER"){
                    levelNumber = -1;
                    stageStartTime = mm;
                    stage = "WIN";
                }
            }else{
                if(lastInputTime+TIMEOUT < mm){
                    stage = "SCREENSAVER";
                }
            }
            if(stage == "SCREENSAVER"){
                // TODO: activate screensaver
            }else if(stage == "PLAY"){
                // PLAYING
                if(attacking && attackMillis+ATTACK_DURATION < mm) attacking = 0;
                // If not attacking, check if they should be
                if(!attacking && joystickWobble > ATTACK_THRESHOLD){
                    attackMillis = mm;
                    attacking = 1;
                }
                // If still not attacking, move!
                playerPosition += playerPositionModifier;
                if(!attacking){
                    int moveAmount = (joystickTilt/6.0);
                    if(DIRECTION) moveAmount = -moveAmount;
                    moveAmount = constrain(moveAmount, -MAX_PLAYER_SPEED, MAX_PLAYER_SPEED);
                    playerPosition -= moveAmount;
                    if(playerPosition < 0) playerPosition = 0;
                    if(playerPosition >= 1000 && !boss.Alive()) {
                        // Reached exit!
                        levelComplete();
                        return;
                    }
                }
                if(inLava(playerPosition)){
                    die();
                }
                // Ticks and draw calls
                tickConveyors();
                tickSpawners();
                tickBoss();
                tickEnemies();
            }else if(stage == "DEAD"){
                // DEAD
                if(!tickParticles()){
                    loadLevel();
                }
            }else if(stage == "WIN"){
                // LEVEL COMPLETE
                if(stageStartTime+1200 <= mm){
                    nextLevel();
                }
            }else if(stage == "COMPLETE"){
                if(stageStartTime+5500 <= mm){
                    nextLevel();
                }
            }else if(stage == "GAMEOVER"){
                // GAME OVER!
                stageStartTime = 0;
            }
        }
};