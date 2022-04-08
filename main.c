#include <gb/gb.h>
#include <stdio.h>
#include "gbt_player.h"

#define MapSizeX 128

extern unsigned char tile_data[];
extern unsigned char map_data1[];
extern unsigned char map_data2[];
extern unsigned char map_data3[];
extern unsigned char map_data4[];
extern unsigned char map_data5[];
extern unsigned char map_data6[];
extern unsigned char map_data7[];
extern unsigned char map_data8[];
extern unsigned char map_data9[];
extern unsigned char char_tiles[];
extern unsigned char menu_tiles[];
extern const unsigned char * song_Data[];

void init();
void menuScreen();
void scroll_right();
void char_movement();
void deathAnim();
void dangerAnim();
unsigned char get_block_at_pos(INT16 hX,INT16 hY);
void removeBlock(INT16 hX, INT16 hY);
void setBlock(UBYTE hX, UBYTE hY, unsigned char hBlock);
UINT8 blockCat(unsigned char blockID);
UINT8 collision_down();
UINT8 collision_up();
UINT8 collision_right();
void collision_inside();
void increaseCoinCount();
void drawScore();
void drawHigh();
void hideHigh();
void draw_title();
void incCurMap();
void nextLevel();

// memory bank of current level
unsigned char curBank;

// x position of leftmost block on screen from map beginning
UINT16 blockScroll;

// x position of leftmost block on screen from memory beginning 
UINT8 blockScrollMem;

// pixels since last full block on screen
UINT8 pixelScroll;

// helper variables
UINT16 i, j;
UWORD counter;

// how long to slide for
INT8 sliding;
// state of animation
UINT8 anim_state;

// player position arrays: blocks + pixels from start, blocks from start of graphics memory
UINT16 playerPosBlocks[2];
INT8 playerPosPixels[2];

UINT16 playerPosBlocksMem;

// save from last level -> start at same xPos
UINT8 setBackBlocks;
UINT8 setBackPixels;

// how long should the player jump
UINT8 jump_counter;

// is the player dead?
UINT8 isDead;

// animation state of danger
INT8 dangerAnimState;
// time until next animation change
INT8 dangerAnimationTimer;

// collected coins (=score); one digit in each array field -> easier to display score
UINT8 coinCount[5];

// pointer to read/write saves
UINT8 *RAMPtr;

// pointer to current map
unsigned char *map_data;

// index of current map
UINT8 curMap;

// number of maps
UINT8 mapCount;

// main function
void main(){
	mapCount = 9;	
	
	// initialize variables; draw initial screen
	init();
	
	// get save address
	ENABLE_RAM_MBC1;
	
	RAMPtr = (UINT8 *)0xa000;
	
	// if no highscore exists: write zeroes
	if ((RAMPtr[5]!=37u) || (RAMPtr[6]!=42u)){
		RAMPtr[5]=37u;
		RAMPtr[6]=42u;
		for (i=0; i!=5; i++){
			RAMPtr[i] = 0u;
		}
	}
	
	DISABLE_RAM_MBC1;
	
	menuScreen();
		
	// repeat until battery runs out
	while(1){
		
		while(!isDead){		
			wait_vbl_done();
			
			drawScore();
			
			dangerAnim();
			
			scroll_right();
			
			char_movement();
			
			incCurMap();
			
			gbt_update();
			
			SWITCH_ROM_MBC1(2);
		}
		
		// update score (if collected coin tick before death)
		wait_vbl_done();
		
		drawScore();		
		
		// stop music
		gbt_stop();
		
		// play animation if player is dead
		deathAnim();
		
		// show menu screen if dead
		menuScreen();
	}
	
	DISABLE_RAM_MBC1;
}

void menuScreen(){
	// show menu after death or on start
	
	//reset set back values
	
	setBackBlocks = 0u;
	setBackPixels = 0u;
	
	// save highscore if there is a new one
	
	ENABLE_RAM_MBC1;
	
	RAMPtr = (UINT8 *)0xa000;
	
	i = 0;
	
	while ((i!=5) && (coinCount[i]==RAMPtr[i])){
		i++;
		incCurMap();
	}
	
	// first digit that is different indicates which score is higher
	if ((i!=5) && (coinCount[i]>RAMPtr[i])){
		// write highscore
		for (j=0; j!=5; j++){
			RAMPtr[j] = (UINT8) coinCount[j]; 
		}
	}
		
	DISABLE_RAM_MBC1;
	
	//show highscore
	drawHigh();
	
	draw_title();
	
	// new game if pressed start
	i = 0;
	
	while(i!=J_START){
		wait_vbl_done();
		i = joypad();
		incCurMap();
	}
	
	// don't display highscore during game
	hideHigh();
	
	init();
	
	for (i=0; i!=5; i++){
		coinCount[i] = 0u;
	}
	
	// start music

	gbt_play(song_Data, 2, 7);
	gbt_loop(1);
}

void nextLevel(){
	// load next level
	
	init();
	
	playerPosBlocks[0] -= setBackBlocks;
	playerPosBlocksMem -= setBackBlocks;
	
	//reset set back values
	
	setBackBlocks = 0u;
	setBackPixels = 0u;
}

void incCurMap(){
	// increase current map index (pseudo-randomizer)
	
	curMap++;
	
	if (curMap==mapCount){
		curMap = 0;
	}
}

void char_movement(){
	UINT8 hCollisionDown;
	UINT8 hCollisionUp;
	
	// check if button was pressed
	i = joypad();
	
	// get if collision at head
	hCollisionUp = collision_up();
	
	// get if collision at feet
	hCollisionDown = collision_down();
			
	// jump
	if ((i == J_UP) || (i == J_A)) {
		if(hCollisionDown){
			jump_counter = 30;
			incCurMap();
		}	
	}
	
	// slide
	if ((i == J_DOWN) || (i == J_B)) {
		// 50 ticks until standing up
		sliding = 50;	
		
		// load sliding sprites
		set_sprite_tile(0,3);
		set_sprite_tile(1,7);
	}
	
	// if collision above, currently jumping -> stop jumping
	if (jump_counter && hCollisionUp){
		jump_counter = 0;
	}
	// go up if no collision
	else if (jump_counter>5){
		playerPosPixels[1]--;
		incCurMap();
	}
	// fall if nothing below and not still in jump
	else if (!hCollisionDown && !jump_counter){
		playerPosPixels[1]++;
	}
	
	// decrease time left in jump
	if (jump_counter){
		jump_counter--;
	}
	
	// don't move right if collision on right side
	if (!collision_right()){
		playerPosPixels[0]++;
		incCurMap();
	}
	else{
		setBackPixels++;
		
		if (setBackPixels==8u){
			setBackBlocks++;
			setBackPixels = 0u;
		}
	}
	
	// inc block counter if block size reached
	if (playerPosPixels[0]==8){
		playerPosPixels[0] = 0;
		playerPosBlocks[0]++;
		playerPosBlocksMem++;
		
		if (playerPosBlocksMem==32){
			playerPosBlocksMem = 0;
		}
	}	
	
	if (playerPosPixels[1]==8){
		playerPosPixels[1] = 0;
		playerPosBlocks[1]++;
	}	
	else if (playerPosPixels[1]==-1){
		playerPosPixels[1] = 7;
		playerPosBlocks[1]--;
	}
	
	// check if coins collected or touched obstacle
	collision_inside();
	
	// find out if player died by time stream or whatever or by falling
	if ((playerPosBlocks[0]==blockScroll+1) || (playerPosBlocks[1] == 19)){
		isDead = 1;
		return;
	}	

	// next level if end reached
	if (playerPosBlocks[0]==MapSizeX-2){
		nextLevel();
		return;
	}	
	
	// draw char
	if (sliding){
		// decrease time left for slide
		sliding--;
		
		// don't stand up from sliding if block over head
		if (!sliding && hCollisionUp){
			sliding = 1;
			incCurMap();
		}
		
		// set in lying position
		if(sliding){
			move_sprite(0, (playerPosBlocks[0]-blockScroll-1)*8 + playerPosPixels[0]-pixelScroll, playerPosBlocks[1]*8+playerPosPixels[1]);
			move_sprite(1, (playerPosBlocks[0]-blockScroll)*8 + playerPosPixels[0]-pixelScroll, playerPosBlocks[1]*8+playerPosPixels[1]);
		}
		// change sprite if just stood up
		else{
			set_sprite_tile(0,0);
			set_sprite_tile(1,4);
		}
	}
	else{
		anim_state++;
		
		switch(anim_state){
			case 16:
				set_sprite_tile(0,0);
				set_sprite_tile(1,4);
				break;
			case 32:
				set_sprite_tile(0,2);
				set_sprite_tile(1,6);
				break;
			case 48:
				set_sprite_tile(0,0);
				set_sprite_tile(1,4);
				break;
			case 64:
				set_sprite_tile(0,1);
				set_sprite_tile(1,5);
				anim_state = 0;
				break;				
		}
		
		// set in upright position
		move_sprite(0, (playerPosBlocks[0]-blockScroll)*8 + playerPosPixels[0]-pixelScroll, (playerPosBlocks[1]-1)*8+playerPosPixels[1]);
		move_sprite(1, (playerPosBlocks[0]-blockScroll)*8 + playerPosPixels[0]-pixelScroll, playerPosBlocks[1]*8+playerPosPixels[1]);
	}
	SHOW_SPRITES;
}

void deathAnim(){
	// animation of player flying up after death
	
	// set right sprites
	set_sprite_tile(0,0);
	set_sprite_tile(1,4);
	
	while(playerPosBlocks[1]!=0){
		playerPosPixels[1]--;
		
		if (playerPosPixels[1]==-1){
			playerPosPixels[1] = 7;
			playerPosBlocks[1]--;
		}
		
		wait_vbl_done();		
		
		// draw in upright position
		move_sprite(0, (playerPosBlocks[0]-blockScroll)*8 + playerPosPixels[0]-pixelScroll, (playerPosBlocks[1]-1)*8+playerPosPixels[1]);
		move_sprite(1, (playerPosBlocks[0]-blockScroll)*8 + playerPosPixels[0]-pixelScroll, playerPosBlocks[1]*8+playerPosPixels[1]);		
		
		SHOW_SPRITES;
	}
}

unsigned char get_block_at_pos(INT16 hX,INT16 hY){
	// return block id at given coordinates	
	unsigned char hReturn;
	
	hX-=1;
	hY-=1;
	
	if (hX<0){
		hX+=32;
	}
	else if (hX > 32){
		hX -= 32;
	}
	
	get_bkg_tiles(hX, hY, 1, 1, &hReturn);
	
	return hReturn;
}

void draw_title(){
	// draw title screen
	
	UBYTE hInt;
	
	wait_vbl_done();
	
	hInt = 6;
	for (counter=83; counter!=93; counter++){
		setBlock(hInt,6,counter);
		hInt++;
	}
	wait_vbl_done();
	hInt = 6;
	for (counter=93; counter!=103; counter++){
		setBlock(hInt,7,counter);
		hInt++;
	}
	wait_vbl_done();
	hInt = 6;
	for (counter=103; counter!=112; counter++){
		setBlock(hInt,8,counter);
		hInt++;
	}
	wait_vbl_done();
	hInt = 10;
	for (counter=112; counter!=114; counter++){
		setBlock(hInt,9,counter);
		hInt++;
	}
	wait_vbl_done();
	hInt = 10;
	for (counter=114; counter!=116; counter++){
		setBlock(hInt,10,counter);
		hInt++;
	}
	wait_vbl_done();
	hInt = 8;
	for (counter=116; counter!=122; counter++){
		setBlock(hInt,11,counter);
		hInt++;
	}
	wait_vbl_done();
	hInt = 8;
	for (counter=122; counter!=128; counter++){
		setBlock(hInt,12,counter);
		hInt++;
	}
}

void setBlock(UBYTE hX, UBYTE hY, unsigned char hBlock){
	// set the block at the given screen coordinates (draw title)
	
	hX += blockScrollMem;
	
	hX-=1;
	hY-=1;
	
	if (hX > 32){
		hX -= 32;
	}
	
	// for some reason, this only seems to work with UBYTES (else there's corruptions on vram edges) even though it's working in removeBlock
	set_bkg_tiles(hX, hY, 1, 1, &hBlock);
	
	SHOW_BKG;
}

void removeBlock(INT16 hX, INT16 hY){
	// remove the block at the given coordinates (coin collected)
	
	unsigned char hBlock;
	
	hBlock = 0x00;
		
	hX-=1;
	hY-=1;
	
	if (hX<0){
		hX+=32;
	}
	else if (hX > 31){
		hX -= 32;
	}
	
	set_bkg_tiles(hX, hY, 1, 1, &hBlock);
}

UINT8 blockCat(unsigned char blockID){
	// returns type of block: 0 for block, 1 for air, 2 for coin, 3 for obstacle
	if (blockID == 0x00){
		return 1;
	}
	if (blockID == 0x02){
		return 2;
	}
	if (blockID == 0x01){
		return 3;
	}
	
	return 0;
}

void collision_inside(){
	// coin or obstacle collision
	
	// player is standing
	if(!sliding){
		// player x position exactly matches block position
		if (!playerPosPixels[0]){
			// player y position exactly matches block position
			if (!playerPosPixels[1]){
				if ((blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==3) 
				|| (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2))==3)){
					isDead = 1;
					return;
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-1);
				} 
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-2);
				}
			}
			else{
				if ((blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==3) 
				|| (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))==3)){
					isDead = 1;
					return;
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-1);
				} 
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-2);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]);
				}				
			}
		}
		else{
			// player y position exactly matches block position
			if (!playerPosPixels[1]){
				if ((blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==3) 
				|| (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-2))==3)){
					isDead = 1;
					return;
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-1);
				} 
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-2);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem+1,playerPosBlocks[1]-1);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-2))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem+1,playerPosBlocks[1]-2);
				}
			}
			else{
				if ((blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==3) 
				|| (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-2))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]))==3)){
					isDead = 1;
					return;
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-1);
				} 
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-2);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem+1,playerPosBlocks[1]-1);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-2))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem+1,playerPosBlocks[1]-2);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem+1,playerPosBlocks[1]);
				}
				
			}
		}
	}
	else{
		// player x position exactly matches block position
		if (!playerPosPixels[0]){
			// player y position exactly matches block position
			if (!playerPosPixels[1]){
				if ((blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==3) 
				|| (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-1))==3)){
					isDead = 1;
					return;
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-1);
				} 
				if (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem-1,playerPosBlocks[1]-1);
				}
			}
			else{
				if ((blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==3) 
				|| (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-1))==3)){
					isDead = 1;
					return;
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-1);
				} 
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]);
				}		
				if (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem-1,playerPosBlocks[1]);
				}	
				if (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem-1,playerPosBlocks[1]-1);
				}		
			}
		}
		else{
			// player y position exactly matches block position
			if (!playerPosPixels[1]){
				if ((blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==3) 
				|| (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-1))==3)){
					isDead = 1;
					return;
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-1);
				} 
				if (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem+1,playerPosBlocks[1]-1);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem-1,playerPosBlocks[1]-1);
				}
			}
			else{
				if ((blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==3) 
				|| (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-1))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))==3)
				|| (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]))==3)){
					isDead = 1;
					return;
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]-1);
				} 
				if (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem+1,playerPosBlocks[1]-1);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem,playerPosBlocks[1]);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem+1,playerPosBlocks[1]);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem-1,playerPosBlocks[1]);
				}
				if (blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-1))==2){
					increaseCoinCount();
					removeBlock(playerPosBlocksMem-1,playerPosBlocks[1]-1);
				}
				
			}
		}
	}
}

UINT8 collision_down(){
	// check if player collides with ground
	
	// no collision possible if between blocks
	if (playerPosPixels[1]){
		return 0;
	}
	
	// more surface area if sliding
	if (!sliding){
		// less blocks to collide with if player stands right at the edge
		if (!playerPosPixels[0]){
			if (!blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))){
				return 1;
			}
		}
		else if ((!blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1])))){
			return 1;
		}
	}
	else{
		// less blocks to collide with if player stands right at the edge
		if (!playerPosPixels[0]){
			if ((!blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1])))){
				return 1;
			}
		}
		else if ((!blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1])))
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1])))){
			return 1;
		}
	}
	return 0;
}

UINT8 collision_up(){
	// check if player head collides
	
	// no collision possible if between blocks
	if (playerPosPixels[1]){
		return 0;
	}
	
	// more surface area if sliding
	if (!sliding){
		// less blocks to collide with if player stands right at the edge
		if (!playerPosPixels[0]){
			if (!blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-3))){
				return 1;
			}
		}
		else if ((!blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-3))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-3)))){
			return 1;
		}
	}
	else{
		// less blocks to collide with if player stands right at the edge
		if (!playerPosPixels[0]){
			if ((!blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-2))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2)))){
				return 1;
			}
		}
		else if ((!blockCat(get_block_at_pos(playerPosBlocksMem-1,playerPosBlocks[1]-2))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem,playerPosBlocks[1]-2)))
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-2)))){
			return 1;
		}
	}
	return 0;
}

UINT8 collision_right(){
	// check if right side collides
	
	// no collision possible if between blocks
	if (playerPosPixels[0]){
		return 0;
	}
	
	// less surface area if sliding
	if (!sliding){
		// less blocks to collide with if player stands right at the edge
		if (!playerPosPixels[1]){
			if ((!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-2)))){
				return 1;
			}
		}
		else if ((!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-2))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1)))
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1])))){
			return 1;
		}
	}
	else{
		// less blocks to collide with if player stands right at the edge
		if (!playerPosPixels[1]){
			if (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))){
				return 1;
			}
		}
		else if ((!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1]-1))) 
			|| (!blockCat(get_block_at_pos(playerPosBlocksMem+1,playerPosBlocks[1])))){
			return 1;
		}
	}
	return 0;
}

void scroll_right(){
	// scroll right if not eof
	if (blockScroll != MapSizeX - 22){
		scroll_bkg(1, 0);
		pixelScroll++;
	}
	
	// load new tiles if scrolled one block (= 8 pixels)
	if (pixelScroll == 8){
		blockScroll++;
		pixelScroll = 0;
		
		blockScrollMem++;
		if (blockScrollMem == 32){
			blockScrollMem = 0;
		}
		
		counter = blockScroll + 21;
		j = counter%32;
		
		SWITCH_ROM_MBC1(curBank);
	
		for (i = 0; i != 18; i++){
			set_bkg_tiles(j, i, 1, 1, &(map_data+counter));
			counter = counter + MapSizeX;
		}
	}
	SHOW_BKG;
}

void dangerAnim(){
	// animate timesteam or whatever the stuff on the left is (communism)
	
	if (dangerAnimationTimer==20){
		j = 8 + dangerAnimState;
		
		for(i = 2; i!=20; i++){
			set_sprite_tile(i, j);
			j++;
			if (j==12){
				j = 8;
			}
		}
		
		dangerAnimState++;
		if (dangerAnimState==4){
			dangerAnimState=0;
		}
		
		dangerAnimationTimer = 0;
	}	
	
	dangerAnimationTimer++;
}

void increaseCoinCount(){
	// increase score digit-wise
	
	coinCount[4]++;
	for (i=0; i!=4; i++){
		if (coinCount[4-i]==10u){
			coinCount[4-i] = 0u;
			coinCount[3-i]++;
		}
	}
	
	// scores over 99999 unlikely, but fix overflow if it happens
	if (coinCount[0]==10u){
		coinCount[0] = 0u;
	}
}

void drawScore(){
	// draw the current score in the top right corner
	
	for(i=0; i!=5; i++){
		set_sprite_tile(20+i, coinCount[i]+12);
	}
	
	SHOW_SPRITES;
}

void drawHigh(){
	// draw the highscore below the current score
	wait_vbl_done();	
	
	ENABLE_RAM_MBC1;
	
	RAMPtr = (UINT8 *)0xa000;
	
	set_sprite_tile(25, 22);
	set_sprite_tile(26, 23);
	set_sprite_tile(27, 24);
	set_sprite_tile(28, 22);
		
	for(i=0; i!=5; i++){
		set_sprite_tile(29+i, RAMPtr[i]+12);
	}	
	
	DISABLE_RAM_MBC1;
	
	SHOW_SPRITES;
}

void hideHigh(){
	// hide the highscore during game
	wait_vbl_done();	
	
	for(i=25; i!=34; i++){
		set_sprite_tile(i, 25);
	}	
}

void init() {
	// initialize variables
	
	move_bkg(0,0);
	
	dangerAnimState = 0;
	dangerAnimationTimer = 0;
	
	isDead = 0;	
	
	blockScroll = 0;
	blockScrollMem = 0;
	pixelScroll = 0;
	i = 0;
	j = 0;
	counter = 0;	
	
	jump_counter = 0;
	
	sliding = 0;
	anim_state;
	
	// place player roughly in middle of screen
	playerPosBlocks[0] = 10;
	playerPosBlocks[1] = 8;
	
	playerPosBlocksMem = 10;
	
	playerPosPixels[0] = 4;
	playerPosPixels[1] = 0;
	
	wait_vbl_done();
	disable_interrupts();
	
	DISPLAY_OFF;
	HIDE_SPRITES;
	HIDE_WIN;
	HIDE_BKG;
	
	SWITCH_ROM_MBC1(2);
	
	// load sprites
	set_sprite_data(0, 26, &char_tiles);
	
	// load blocks
	set_bkg_data(0, 83, &tile_data);
	
	// load menu image
	set_bkg_data(83, 105, &menu_tiles);
	
	counter = 0;
	
	// load character sprites
	set_sprite_tile(0,0);
	set_sprite_tile(1,4);
	
	// load danger sprites
	j = 16;
	for (i = 2; i!=20; i++){
		set_sprite_tile(i,8);
		move_sprite(i, 8, j);
		j+=8;
	}
	
	// set score sprites to 0
	j = 126;
	for (i = 20; i != 25; i++){
		set_sprite_tile(i, 12);
		move_sprite(i, j, 18);
		j+=8;
	}
	
	// set high score sprites to invisible
	j = 86;
	for (i = 25; i != 29; i++){
		set_sprite_tile(i, 25);
		move_sprite(i, j, 28);
		j+=8;
	}
	j = 126;
	for (i = 29; i != 34; i++){
		set_sprite_tile(i, 25);
		move_sprite(i, j, 28);
		j+=8;
	}
	
	
	// set pointer to current map based on pseudo-random index
	// set correct memory bank every time map_data is referenced
	switch(curMap){
		case 0:
			curBank = 3;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data1;
			break;
		case 1:	
			curBank = 3;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data2;
			break;
		case 2:	
			curBank = 3;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data3;
			break;
		case 3:	
			curBank = 3;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data4;
			break;
		case 4:	
			curBank = 3;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data5;
			break;
		case 5:	
			curBank = 4;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data6;
			break;
		case 6:	
			curBank = 4;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data7;
			break;
		case 7:	
			curBank = 4;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data8;
			break;
		case 8:	
			curBank = 4;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data9;
			break;
		default:
			curBank = 3;
			SWITCH_ROM_MBC1(curBank);
			map_data = map_data1;
			break;
	}
	
	// draw initial screen
	for (i = 0; i != 18; i++){
		set_bkg_tiles(0, i, 22, 1, &(map_data+counter));
		counter = counter + MapSizeX;
	}
	
	//show the map
	
	SHOW_SPRITES;
	SHOW_BKG;
	DISPLAY_ON;

	set_interrupts(VBL_IFLAG);
	
	enable_interrupts();	
}