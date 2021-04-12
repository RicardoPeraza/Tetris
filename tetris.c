#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#include "stdint.h"
#include "SDL/SDL_mixer.h"
#include <string.h>





#define tamanio_pieza 16
#define NBpiezas 7
#define X_tamanio_pantalla 700
#define Y_tamanio_pantalla 400
#define Nfilas 21
#define Ncolumas 12
#define STARTXzonaJuego 4
#define STARTYzonaJuego 2
#define DOUBLEFOR(i,imax,j,jmax) for(i=0;i<imax;i++) for(j=0;j<jmax;j++)
#define MAX 100

int GRAVEDAD=500;
int PAUSE=0;// estado 0 es continuar y estado 1 es pausa
int puntaje=0;
int nivel=0;
int bonoextra=0;
int continuar=1;
char usuario[MAX];
char jugador[MAX];
int t=0;



void borrar(SDL_Surface **pantalla,int x ,int y , int alto , int ancho );
int pixel(SDL_Surface* srf,int x,int y,Uint32 color); 

//***********************************************************************************************************
//leer cadena desde consola
void leercadena(char *cad, int n){
int i=0;
char car;
while(1){
car = getchar();
if(car == '\n') break;
if(i<n) cad[i++]=car;
}
cad[i] = '\0';
}



//********************************************************************************************************





/*
La funcion limpiarArreglo lo que hace es limpiar los caracteres del archivo de texto donde se almacena los puntajes
de los ganadores la funcion limpia hasta que se encuentre el ultima posicion de char que es \0
*/

void limpiarArreglo(char cadena[]){
	int i;
	for(i=0;i<MAX;i++)
		cadena[i]='\0';
}

//***********************************************************************************
//archivos
//aqui es donde escribo los puntajes en el text de puntajes de los ganadores

void archivos(){

	FILE *f = NULL;//abro el File
	uint32_t puntaje2 =puntaje; //le paso la variable global puntaje 
	char caracter_puntaje[16]={0};
	f = fopen("tetris.txt", "a");//donde guardo los puntajes

	if(f == NULL){
		printf("Problemas para leer el archivo \n");
	}



	sprintf(caracter_puntaje,"%d" , puntaje2);//paso la variable puntaja de int a string
	fprintf(f, "%s-\n",  strcat( usuario , strcat( caracter_puntaje , " " ) )  );/*Escribimos en el archivo*/
	//fputs(strcat(usuario,caracter_puntaje ),f);//le envio el puntaje al archivo de texto
	fclose(f); // cierro la funcion de archivos

}
//**************************************************************************************


//creo mi estruc de la posicion x , y y estado de rotacion
struct estados{
    int x,y,rotacion;
};
// mas estruct con variables que utilizo 
struct estruc_tetris{
    SDL_Surface* tiles;//pantalla de juego
    Uint8 zona[Ncolumas][Nfilas];//zona de juego
    Uint8 piezas[NBpiezas][4][4][4];//matriz de las piezas
    int numpiezas,numpiezasnext;
    struct estados scu;
    struct estados sca;
    int termino_juego,gravedad; //game over con 1 termina
    Uint32 nextmove;
};

typedef struct estruc_tetris Tetris;

int pixel(SDL_Surface* srf,int x,int y,Uint32 color){
    Uint32 *p = (Uint32*)(((Uint8*)srf->pixels) + y * srf->pitch + x * 4);
    *p = color;
    return 0;
}

/*
aqui creo el juego de fichas le creo una superficie  le doy el tamanio 
le pongo los colores ojo {0,0,0} es negro aqui lo pinto todo de negro
ya lo demas son los colores que le pongo a cada pieza de la zona de juego
efecto_colores_pieza  es para que se vea la pieza tridimencional con otros colores
para que no se vea plano los colores explicacion tiene 3 colores distintos  un esquema podria ser
333333333333331
333333333333311
222222222222211
los colores son color luz 2 color medio 3 color oscuro
33333333333331
se puede escribir en binario asi
111111111111111111111111111101
en exadecimal es FFFFFFFD
despues empiezo a construir cada matriz de colores para las piezas con for

*/

int construircolorespiezas(SDL_Surface* srf){//2 es 128,128,128 del marco
    Uint32 colores[NBpiezas+2][3]={{0,0,0},
        {192,7,7},{255,0,0},{0,255,0},{0,0,255},
        {255,255,0},{255,0,255},{0,255,255},{255,128,0}
    };
    Uint32 efectos_colores_piezas[tamanio_pieza]={
        0xFFFFFFFD,0xFFFFFFF5,0xFAAAAAA5,0xFAAAAAA5,
        0xFAAAAAA5,0xFAAAAAA5,0xFAAAAAA5,0xFAAAAAA5,
        0xFAAAAAA5,0xFAAAAAA5,0xFAAAAAA5,0xFAAAAAA5,
        0xFAAAAAA5,0xFAAAAAA5,0xD5555555,0x55555555
    };
    int i,j,k;
    SDL_LockSurface(srf);
    for(k=0;k<NBpiezas+2;k++){
        Uint32 LocalPalette[4];
        for(i=0;i<4;i++)
            LocalPalette[i] = SDL_MapRGBA(srf->format,(colores[k][0]/3)*i,(colores[k][1]/3)*i,(colores[k][2]/3)*i,0);
        DOUBLEFOR(i,tamanio_pieza,j,tamanio_pieza){
            int ndat = j*tamanio_pieza+i;
            Uint32 ind = ((efectos_colores_piezas[ndat/16]<<((ndat%16)*2))&0xC0000000)>>30;
            pixel(srf,k*tamanio_pieza+i,j,LocalPalette[ind]);
        }
    }
    SDL_UnlockSurface(srf);
   
    return 0;
}


/*
aqui es donde realizo mi matriz de piezas es una matriz de 4x4
la logica es hacer cada posicion de pieza en una matriz y sus diferentes tipos de rotacion en si don 7 piezas con 4 rotaciones
distintas pero el cuadrado no tiene rotacion por comodidad se deja igual siempre
en la posicion 0xC600
en binario es 1100 0110 0000 0000
en formato del tetris es
1100
0110
0000
0000
aqui forme la pieza z  los demas estados son los diferentes tipos de rotacion que hay ahora si llamo cuadradito solo le pregunto la 
posicion y la rotacion ya esta hecha 
*/



int construirpiezas(Tetris* T){
    Uint16 cuadradito[NBpiezas][4] = {
        {0xC600,0x2640,0xC600,0x2640},
        {0x6C00,0x4620,0x6C00,0x4620},
        {0x8E00,0x6440,0x0E20,0x44C0},
        {0x6600,0x6600,0x6600,0x6600},
        {0x4E00,0x4640,0x0E40,0x4C40},
        {0x0F00,0x2222,0x0F00,0x2222},
        {0x2E00,0x4460,0x0E80,0xC440}
    };
    int i,j,k,l;
    DOUBLEFOR(i,NBpiezas,j,4)
        DOUBLEFOR(k,4,l,4)
        {
            int dat = ((cuadradito[i][j]<<(l*4+k))&0x8000)>>15;
            T->piezas[i][j][k][l] = dat*(i+2);
        }
    return 0;
}

/*
Aqui es donde genero la piezas aleatorias las le pongo la posicion la gravedad a las piezas etc
*/

int atributos_piezas(Tetris* T){
    T->numpiezas = T->numpiezasnext;
    T->numpiezasnext = rand()%NBpiezas;
    T->scu.rotacion = 0;
    T->scu.x = (Ncolumas-4)/2;
    T->scu.y = 0;
    T->nextmove = SDL_GetTicks()+T->gravedad;
    return 0;
}

int tetris_inicio(Tetris* T){
    int i;


    T->numpiezasnext = T->termino_juego = T->nextmove = 0;
    T->gravedad = GRAVEDAD;
    atributos_piezas(T);
    atributos_piezas(T);
    T->tiles = SDL_CreateRGBSurface(0,(NBpiezas+2)*tamanio_pieza,tamanio_pieza,32,0,0,0,0);//creo pantallla de juego
    construircolorespiezas(T->tiles);
    construirpiezas(T);
//lleno de 0 todo
    memset(T->zona,0,Ncolumas*Nfilas);
    for(i=0;i<Ncolumas;i++)
        T->zona[i][Nfilas-1] = 1; // lineas de proteccion de la base
    for(i=0;i<Nfilas;i++)
    {
        T->zona[0][i] = 1; // lineas de proteccion de los lados
       T->zona[Ncolumas-1][i] = 1;
    }
    return 0;
}

//aqui es donde termina tetris 

int tetris__fin(Tetris* T){

    SDL_FreeSurface(T->tiles);
	

//cuando termina el juego muestra esta pantalla unos segundos de juego terminado

const int ancho = X_tamanio_pantalla;
const int alto = Y_tamanio_pantalla;  /* Propiedades de Ventana */
const int bpp=32;

 SDL_Surface* screen3 = NULL; //ventana
    SDL_Surface* texto3 = NULL; //texto
    TTF_Font *fuente = NULL;  //fuente para escribir
    SDL_Color blanco = { 150, 180, 200}; //color
    SDL_Rect coordenadas;
    coordenadas.x=300;
    coordenadas.y=150;

Mix_Chunk *effect1_termino_juego; // hago efecto de rotacion
    Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,4096);
    effect1_termino_juego=Mix_LoadWAV("termino_juego.wav");
 

    SDL_Init( SDL_INIT_EVERYTHING ); //Inicia SDL
    
// Inicializamos SDL_ttf
	if (TTF_Init() < 0) {
		printf("No se pudo iniciar SDL_ttf: %s\n",SDL_GetError());
		return 1;
	}

	atexit(TTF_Quit);


    screen3 = SDL_SetVideoMode(ancho, alto, bpp, SDL_SWSURFACE );

    if (screen3==NULL)
    {
        return 1; //Error al iniciar la ventana

    }

    fuente=TTF_OpenFont( "dalila.ttf", 25 ); //colocamos la fuente y su tamaño, la fuente debe estar en el directorio para funcionar
	//nota, escogi esa letra porque va mas con el tetris, hay varios fonts pero creo que ese cuela bien

    texto3=TTF_RenderText_Solid(fuente, "GAME OVER!", blanco); //generamos el texto a mostrar
Mix_PlayChannel(-1,effect1_termino_juego,0);
    SDL_BlitSurface(texto3, NULL, screen3, &coordenadas);
texto(&screen3,150,240,"Puntaje");
texto_puntaje(&screen3);
texto(&screen3,150,280,"Jugador con sus mejores puntuaciones");
texto(&screen3,150,300,usuario);
    SDL_Flip(screen3);
    SDL_Delay(5000);
Mix_FreeChunk(effect1_termino_juego);
Mix_CloseAudio();


// destruimos la fuente de letra
	TTF_CloseFont(fuente);

    



//*********************************************************************************************************


//********************************************************************************************************
//es para continuar y al terminar el juego

//****************************************************************************************
//menu de pantalla
int i;
SDL_Surface *screen, *icon;
	const int width = X_tamanio_pantalla; //Ancho de pantalla
	const int height = Y_tamanio_pantalla;//Alto de pantalla
	screen = SDL_SetVideoMode(width,height,32,SDL_SWSURFACE);
	Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,4096);
	Mix_Music *music;
	music = Mix_LoadMUS("menutheme.mp3"); //Tema del menu
	//music = Mix_LoadMUS("tetris.mp3"); //Tema del menu
	Mix_PlayMusic(music,-1); //repetir indefinidamente
	TTF_Font *font;
	TTF_Init();
	font = TTF_OpenFont("dalila.ttf",25);
	SDL_Color color = {0,0,0}; //Fondo negro
	SDL_Event event;
	Uint32 times;
	int x, y;
	char *mensaje[3] = {"Iniciar", "Puntuacion", "Salir"};
	SDL_Surface *menus[3];
	int selected[3] = {0,0,0};
	SDL_Color color2[2] = {{255,255,255},{255,0,0}}; //Texto Blanco y rojo

	menus[0] = TTF_RenderText_Solid(font,mensaje[0],color2[0]); //Iniciar
	menus[1] = TTF_RenderText_Solid(font,mensaje[1],color2[0]); //Puntuacion
	menus[2] = TTF_RenderText_Solid(font,mensaje[2],color2[0]); //Salir
    SDL_Rect pos[3];
	//clip_rect.w/2 - menus[0]->clip_rect.w/2 lo que hago es calcular la mitad de la pantalla y restarle lo que ocupa
	//el texto del menu para que quede centrado
	pos[0].x = screen->clip_rect.w/2 - menus[0]->clip_rect.w/2;
	pos[0].y = 150;
	pos[1].x = screen->clip_rect.w/2 - menus[1]->clip_rect.w/2;
	pos[1].y = 190;
	pos[2].x = screen->clip_rect.w/2 - menus[2]->clip_rect.w/2;
	pos[2].y = 230;	
	SDL_FillRect(screen,&screen->clip_rect,SDL_MapRGB(screen->format,0x00,0x00,0x00));
	while(1)
         {
                    times = SDL_GetTicks();
                    while(SDL_PollEvent(&event))
                    {
                            switch(event.type)
                            {
                                    case SDL_QUIT:
					//Libera memoria de las imagenes
                                            SDL_FreeSurface(menus[0]);
                                            SDL_FreeSurface(menus[1]);
											SDL_FreeSurface(menus[2]);
                                            return 1;
                                    case SDL_MOUSEMOTION:
				//Con este, segun el valor de la posicion del mouse cambia el color al texto
                                            x = event.motion.x;
                                            y = event.motion.y;
                                            for( i = 0; i < 3; i += 1) {
                                                    if(x>=pos[i].x && x<=pos[i].x+pos[i].w && y>=pos[i].y && y<=pos[i].y+pos[i].h)
                                                    {
                                                            if(!selected[i])
                                                            {
                                                                    selected[i] = 1;
                                                                    SDL_FreeSurface(menus[i]);
                                                                    menus[i] = TTF_RenderText_Solid(font,mensaje[i],color2[1]);
                                                            }
                                                    }
                                                    else
                                                    {
                                                            if(selected[i])
                                                            {
                                                                    selected[i] = 0;
                                                                    SDL_FreeSurface(menus[i]);
                                                                    menus[i] = TTF_RenderText_Solid(font,mensaje[i],color2[0]);

                                                          }
                                                    }
                                            }
                                            break;
                                    case SDL_MOUSEBUTTONDOWN:
                                            x = event.button.x;
                                            y = event.button.y;
                                            for( i = 0; i < 3; i += 1) {
                                                    if(x>=pos[i].x && x<=pos[i].x+pos[i].w && y>=pos[i].y && y<=pos[i].y+pos[i].h)
                                                    {
                                                            SDL_FreeSurface(menus[0]);
                                                            SDL_FreeSurface(menus[1]);
							    SDL_FreeSurface(menus[2]);



if(i==0){


//***************************************************************************************************
//opcion jugar tetris

puntaje=0;
    Tetris T;
	
    srand((unsigned int)time(NULL)); //para iniciar los numeros aleatorios de la piezas
SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
 SDL_WM_SetCaption("Tetris", "Tetris");
	
    
//Mix_Chunk *effect1;
Mix_Music* music;
Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT	,2,4096);
music=Mix_LoadMUS("tetris.mp3");
//effect1=Mix_LoadWAV("rotacion.wav");
Mix_PlayMusic(music , -1);


// esto es ttf


 SDL_Surface *surface;
    Uint32 rmask, gmask, bmask, amask;

    

    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;


    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 600, 600, 32,
                                   rmask, gmask, bmask, amask);


		surface= SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
    if(surface == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
        exit(1);
    }

//*******************************************************************************************************************
//puntajes 

texto(&surface,300,150,"Nivel");
texto(&surface,300,170,"1");
 SDL_Flip(surface);
    

texto(&surface,300,200,"Puntaje");
texto(&surface,300,240,"0");

texto_pizza(&surface,500,100,"UCA");
texto_alumnos(&surface,420,260,"Desarrolladores: ");
texto_alumnos(&surface,420,290,"Miranda Masin Jose Damian");
texto_alumnos(&surface,420,310,"Ramos Hernandez Marlon David");
texto_alumnos(&surface,420,330,"Perez Avendanio Diego Fernando");
texto_alumnos(&surface,420,350,"Rivera Sanchez Maritza Berenice");
texto_alumnos(&surface,420,370,"Peraza Gonzalez Ricardo Neftali");

//****************************************************************************************************************************
		
		
		tetris_inicio(&T);
   		//pintar_fondo(&T); //cuadros de fondo
    		Juego(&T);
		archivos();
    		tetris__fin(&T);
		
	
//Mix_FreeChunk(effect1);
Mix_FreeMusic(music);
Mix_CloseAudio();

//************************************************************************************************************

}
//pone el menu de mejores puntajes en la segunda ventana
if(i==1){


Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,4096);
	Mix_Music *music;
	music = Mix_LoadMUS("puntaje.mp3"); //Tema del menu
	//music = Mix_LoadMUS("tetris.mp3"); //Tema del menu
	Mix_PlayMusic(music,-1); //repetir indefinidamente


SDL_Surface *mostrar_mejores_puntuaciones;
    Uint32 rmask, gmask, bmask, amask;

    

    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;


    mostrar_mejores_puntuaciones= SDL_CreateRGBSurface(SDL_SWSURFACE, 600, 600, 32,
                                   rmask, gmask, bmask, amask);


		mostrar_mejores_puntuaciones= SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
    if(mostrar_mejores_puntuaciones == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
        exit(1);
    }




//codigo de lic oscar catedratico de la uca tomado de ejemplo  de archivos subido al moodle 
FILE *f;
	char cadena[MAX];
	char n1[MAX], n2[MAX], n3[MAX], n4[MAX], n5[MAX];
	int e1, e2,e3,e4,e5;
	char mostrar_p1[16]={0};
	char mostrar_p2[16]={0};
	char mostrar_p3[16]={0};
	char mostrar_p4[16]={0};
	char mostrar_p5[16]={0};
	//char mostrar_p2[MAX];
	
	
	f= fopen("te.txt", "r");
	
	if(f==NULL){
		return 0;
	}
	
	int i;
	char car;
	
	//Limpiando (quedan vacios) todos los arreglos
	limpiarArreglo(n1);
	limpiarArreglo(n2);
	limpiarArreglo(n3);
	limpiarArreglo(n4);
	limpiarArreglo(n5);
		
	while(!feof(f)){
	//Guardando la información del archivo al programa sobre de la primera persona
		//Obtengo los caracteres (leer del archivo letra por letra) uno a uno.
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n1[i]=car;
		}
		//Obtengo el resto de la linea
		fgets(cadena,MAX,f);
		//atoi es una función que transforma una cadena de caracteres númerica a tipo de datos entero "23" -> 23
		e1= atoi(cadena);
		//Limpiando variables para reutilizar
		car = '0';
		limpiarArreglo(cadena);
		
	//Guardando la información de la segunda persona
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n2[i]=car;
		}
		
		fgets(cadena,MAX,f);
		e2= atoi(cadena);
		
		car = '0';
		limpiarArreglo(cadena);
		
	//Guardando la información de la tercera persona 
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n3[i]=car;
		}
		fgets(cadena,MAX,f);
		e3= atoi(cadena);
		
		car = '0';
		limpiarArreglo(cadena);
	
	//Guardando la información de la cuarta persona	
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n4[i]=car;
		}
		fgets(cadena,MAX,f);
		e4= atoi(cadena);

		//Guardando la información de la quinta persona	
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n5[i]=car;
		}
		fgets(cadena,MAX,f);
		e5= atoi(cadena);


	}
	
	fclose(f);
	
	
sprintf(mostrar_p1,"%d" , e1);
sprintf(mostrar_p2,"%d" , e2);
sprintf(mostrar_p3,"%d" , e3);
sprintf(mostrar_p4,"%d" , e4);
sprintf(mostrar_p5,"%d" , e5);
//sprintf(mostrar_p2,"%d" , e2);
texto(&mostrar_mejores_puntuaciones,300,80,n1);
texto(&mostrar_mejores_puntuaciones,300,100,mostrar_p1);
 SDL_Flip(mostrar_mejores_puntuaciones);
texto(&mostrar_mejores_puntuaciones,300,120,n2);
texto(&mostrar_mejores_puntuaciones,300,140,mostrar_p2);
 SDL_Flip(mostrar_mejores_puntuaciones);
texto(&mostrar_mejores_puntuaciones,300,160,n3);
texto(&mostrar_mejores_puntuaciones,300,180,mostrar_p3);
 SDL_Flip(mostrar_mejores_puntuaciones);
texto(&mostrar_mejores_puntuaciones,300,200,n4);
texto(&mostrar_mejores_puntuaciones,300,220,mostrar_p4);
 SDL_Flip(mostrar_mejores_puntuaciones);

texto(&mostrar_mejores_puntuaciones,200,250,"ES TIEMPO DE SER EL MEJOR!!");
    SDL_Flip(mostrar_mejores_puntuaciones);

SDL_Delay(10000);

		Tetris T;

		tetris_inicio(&T);
   		
    		tetris__fin(&T);

Mix_FreeMusic(music);
Mix_CloseAudio();

}


                                                           return i;
                                                    }
                                            }
                                            break;
                                    case SDL_KEYDOWN:
                                            if(event.key.keysym.sym == SDLK_ESCAPE)
                                            {
                                                    SDL_FreeSurface(menus[0]);
                                                    SDL_FreeSurface(menus[1]);
						    SDL_FreeSurface(menus[2]);
                                                    return 0;
                                            }
                            }
                    }
                    for( i = 0; i < 3; i += 1) {
                            SDL_BlitSurface(menus[i],NULL,screen,&pos[i]);
                    }
                    SDL_Flip(screen);
                    if(1000/30 > (SDL_GetTicks()-times))
                            SDL_Delay(1000/30 - (SDL_GetTicks()-times));
            }



	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();

//*****************************************************************************************************************


	
    return 0;
}

//aqui es donde creo mi rect de las piezas le doy las coordenadas x y y 

int cuadrado(SDL_Surface* srf,int i,int j,int numcuadrado){
    SDL_Rect Rsrc,Rdest;
    Rdest.x = i*tamanio_pieza;
    Rdest.y = j*tamanio_pieza;
    Rsrc.x = numcuadrado*tamanio_pieza;
    Rsrc.y = 0;
    Rsrc.w = tamanio_pieza;
    Rsrc.h = tamanio_pieza;
    SDL_BlitSurface(srf,&Rsrc,SDL_GetVideoSurface(),&Rdest);
    return 0;
}

//
int hacer_piezas(Tetris* T,int x,int y,int npiece,int rotacion,int transparence){
    int i,j;
    DOUBLEFOR(i,4,j,4)
    {
        if (transparence && T->piezas[npiece][rotacion][i][j]==0 && npiece !=16)
            continue;
        cuadrado(T->tiles,x+i,y+j,T->piezas[npiece][rotacion][i][j]);
	if (npiece==4){
	 t=1;
	}
	if (npiece==1){
	 t=0;
	}
	if (npiece==2){
	 t=0;
	}
	if (npiece==3){
	 t=0;
	}
	if (npiece==5){
	 t=0;
	}
	if (npiece==6){
	 t=0;
	}
	if (npiece==7){
	 t=0;
	}
    }

 	
    return 0;
}


//pinta en pantalla las piezas 
int Render(Tetris* T){
    int i,j;
    DOUBLEFOR(i,Ncolumas,j,Nfilas)
        cuadrado(T->tiles,i+STARTXzonaJuego,j+STARTYzonaJuego,T->zona[i][j]);//pone cuadro negro para que no se vea la piza fea
   hacer_piezas(T,STARTXzonaJuego+Ncolumas+2,4,T->numpiezasnext,0,0); // es para poner el cuadro de la siguente pieza;
  
//hacer_piezas(T,STARTXzonaJuego+Ncolumas+2,10,16,0,0);
    hacer_piezas(T,STARTXzonaJuego+T->scu.x,STARTYzonaJuego+T->scu.y,T->numpiezas,T->scu.rotacion,1);//dibuja la pieza en la zona de pantalla del juego principal

    SDL_Flip(SDL_GetVideoSurface());
    SDL_Delay(1);
    return 0;
}
//****************************************************************************************************************************************
#define PIESAGUAR(T,i,j) (T->piezas[T->numpiezas][T->sca.rotacion][i][j])
//valido si no es basura si es distinto de 1 no es basura
int validacion_basura(Tetris* T){
	int i,j;
    DOUBLEFOR(i,4,j,4)
    {
        Uint8 carre = PIESAGUAR(T,i,j);
        if (carre!=0 && T->zona[T->sca.x+i][T->sca.y+j]!=0)
             return 0;
    }
    T->scu = T->sca;
    return 1;
   
}
//hago pieza basura
int basura(Tetris* T){
    int i,j;
    T->sca = T->scu;
    DOUBLEFOR(i,4,j,4)
    {
        Uint8 carre = PIESAGUAR(T,i,j);
        if (carre!=0)
            T->zona[T->sca.x+i][T->sca.y+j] = carre;
    }
    return 0;
}

int eliminar_linea_llena(Tetris* T,int y){ //donde elimina la fila si se ha completado
    int i,j;
	

    
//creo el area de la pantalla donde voy a poner el texto en pantalla y las puntuaciones y el nivel
SDL_Surface *puntuaciones_nivel;
    Uint32 rmask, gmask, bmask, amask;
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
	puntuaciones_nivel = SDL_CreateRGBSurface(SDL_SWSURFACE, 600, 600, 32,
                                   rmask, gmask, bmask, amask);
	puntuaciones_nivel= SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
    		if(puntuaciones_nivel == NULL) {
        		fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
       				exit(1);
    		}

// creo el texto que va a salir en pantalla esto es fijo
	texto(&puntuaciones_nivel,300,150,"Nivel");
	SDL_Flip(puntuaciones_nivel);
        texto(&puntuaciones_nivel,300,200,"Puntaje");
texto_pizza(&puntuaciones_nivel,500,100,"UCA");
texto_alumnos(&puntuaciones_nivel,420,260,"Desarrolladores: ");
texto_alumnos(&puntuaciones_nivel,420,290,"Miranda Masin Jose Damian");
texto_alumnos(&puntuaciones_nivel,420,310,"Ramos Hernandez Marlon David");
texto_alumnos(&puntuaciones_nivel,420,330,"Perez Avendanio Diego Fernando");
texto_alumnos(&puntuaciones_nivel,420,350,"Rivera Sanchez Maritza Berenice");
texto_alumnos(&puntuaciones_nivel,420,370,"Peraza Gonzalez Ricardo Neftali");

	for(i=0;i<Ncolumas;i++){
       		 for(j=y;j>0;j--)
           		 T->zona[i][j] = T->zona[i][j-1];
       			 T->zona[i][0] = 0;
			
			 puntaje++; //a puntaje le sumo los cuadros que tienen y asi saco cada puntaje por cada linea que elimina
		if(i==11){
			if(bonoextra==0 && t==1){
			texto_alumnos(&puntuaciones_nivel,300,310,"T-SPIN"); // T SPING es si destruye fila con la T que es la figura 4
			texto_alumnos(&puntuaciones_nivel,300,330,"+1");//bono es de +1
			bonoextra++;
			}
			if(bonoextra==2 && t==1){
			texto_alumnos(&puntuaciones_nivel,300,310,"T-SPIN DOUBLE"); // T SPING es si destruye fila con la T que es la figura 4
texto_alumnos(&puntuaciones_nivel,300,330,"+2");//bono es de +2
			bonoextra++;
			bonoextra++;
			}
			if(bonoextra==3 && t==1){
texto_alumnos(&puntuaciones_nivel,300,310,"T-SPIN TRIPLE"); // T SPING es si destruye fila con la T que es la figura 4
texto_alumnos(&puntuaciones_nivel,300,330,"+3");//bono es de +1
			bonoextra++;
			bonoextra++;
			bonoextra++;
			}
		
	//si i es igual a 11 es por que se elimino 1 linea le sumo +1 a bono extra por cada linea que elimino
		bonoextra++;//suma cuantas veces se a eliminado una linea
		}
   	 }


    T->zona[0][0] = 1;
    T->zona[Ncolumas-1][0] = 1;
	puntaje=puntaje-2;// a bono extra le quito 2 por que me suma la base asi queda la puntuacion a 10
	puntaje=puntaje+bonoextra; // a puntaje que es de 10 por cada linea que elimino le sumo el bonoextra por cada linea eliminada
	texto_puntaje(&puntuaciones_nivel);// imprimo el puntaje total en pantalla
	
/*
si bonoextra es igual a 1 solo elimino una linea se le suma 10 + 1 = 11 , 10+1
si bonoextra es igual a 2 solo elimino dos lineas se le suma 10 + 10 + 3 =23 , 10+1+10+2
si bonoextra es igual a 3 solo elimino tres lineas se le suma 10+10+10+6=36 , 10+1+10+2+10+3
si bonoextra es igual a 4 solo elimino cuatro lineas se le suma 10+10+10+10+10=50 , 10+1+10+2+10+3+10+4=50

*/

	if(puntaje<100){
		nivel=1;
		texto_nivel(&puntuaciones_nivel);// nivel esta en 1 si el puntaje es menor a 100
		
		}
	if(puntaje>=100 && puntaje <200){
		nivel=2;//nivel esta en 2 si el puntaje es mayor que 100 pero menor a 200
		texto_nivel(&puntuaciones_nivel);
		}
	if(puntaje>=200 && puntaje <300){
		nivel=3;//nivel esta en 2 si el puntaje es mayor que 200 pero menor a 300
		texto_nivel(&puntuaciones_nivel);
		}
		
if(puntaje>=300 ){



   

//cuando termina el juego muestra esta pantalla unos segundos

const int ancho = X_tamanio_pantalla;
const int alto = Y_tamanio_pantalla;  /* Propiedades de Ventana */
const int bpp=32;

 SDL_Surface* screen3 = NULL; //ventana
    SDL_Surface* texto3 = NULL; //texto
    TTF_Font *fuente = NULL;  //fuente para escribir
    SDL_Color blanco = { 150, 180, 200}; //color
    SDL_Rect coordenadas;
    coordenadas.x=300;
    coordenadas.y=150;

    SDL_Init( SDL_INIT_EVERYTHING ); //Inicia SDL
    
// Inicializamos SDL_ttf
	if (TTF_Init() < 0) {
		printf("No se pudo iniciar SDL_ttf: %s\n",SDL_GetError());
		return 1;
	}

	atexit(TTF_Quit);


    screen3 = SDL_SetVideoMode(ancho, alto, bpp, SDL_SWSURFACE );

    if (screen3==NULL)
    {
        return 1; //Error al iniciar la ventana

    }

    fuente=TTF_OpenFont( "dalila.ttf", 25 ); //colocamos la fuente y su tamaño, la fuente debe estar en el directorio para funcionar
	//nota, escogi esa letra porque va mas con el tetris, hay varios fonts pero creo que ese cuela bien

    texto3=TTF_RenderText_Solid(fuente, "YOU WIN!", blanco); //generamos el texto a mostrar

    SDL_BlitSurface(texto3, NULL, screen3, &coordenadas);

    SDL_Flip(screen3);
    SDL_Delay(5000);


// destruimos la fuente de letra
	TTF_CloseFont(fuente);
 T->termino_juego = 1;
    



//*********************************************************************************************************

}



	
    return 0;
}

int eliminar_linea_llenas(Tetris* T,int ystart){ //es para ver cuando ya quedo final una pieza hace la basura
    int i,j;
	
    for(i=0;i<4;i++){
        int cpt = 0;
        if (i+ystart==Nfilas-1)
            break; // para no eliminar las lineas de la bases
        for(j=0;j<Ncolumas;j++)
            if (T->zona[j][i+ystart]!=0)
                cpt++;
        if (cpt==Ncolumas)
            eliminar_linea_llena(T,i+ystart);
		
    }

	bonoextra=0;//reinicia todos los bonosextras a 0

    return 0;

}

int Descenso(Tetris* T){

//PAUSA EN EL JUEGO

	if(PAUSE==0){
	T->sca.y++;
    if (validacion_basura(T)){
        T->nextmove = SDL_GetTicks()+T->gravedad;
	
        return 0;
    }
    else
    {
        if (T->scu.y == 0)
            T->termino_juego = 1;
        basura(T);
        eliminar_linea_llenas(T,T->scu.y);
        atributos_piezas(T);
    }
    return 0;

	}

	else{


SDL_Surface *surface_pause;
    Uint32 rmask, gmask, bmask, amask;
SDL_Event event;
    

    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;


    surface_pause = SDL_CreateRGBSurface(SDL_SWSURFACE, 600, 600, 32,
                                   rmask, gmask, bmask, amask);


		surface_pause= SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
    if(surface_pause == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
        exit(1);
    }

SDL_Flip(surface_pause);
texto(&surface_pause,300,150,"PAUSA");
texto(&surface_pause,150,180,"PRESIONE EL BOTON C PARA CONTINUAR");
 SDL_Flip(surface_pause);



	while(PAUSE==1){

		
		SDL_PollEvent(&event);
			   if(event.type==SDL_QUIT){break;}

			  if(event.type==SDL_KEYDOWN){
				 if(event.key.keysym.sym==SDLK_c){
					PAUSE=0;
					}
				
				}
		}
borrar(&surface_pause,300,150,100,10);
SDL_Flip(surface_pause);
texto(&surface_pause,300,150,"Nivel");
texto_nivel(&surface_pause);
texto_pizza(&surface_pause,500,100,"UCA");
texto_alumnos(&surface_pause,420,260,"Desarrolladores: ");
texto_alumnos(&surface_pause,420,290,"Miranda Masin Jose Damian");
texto_alumnos(&surface_pause,420,310,"Ramos Hernandez Marlon David");
texto_alumnos(&surface_pause,420,330,"Perez Avendanio Diego Fernando");
texto_alumnos(&surface_pause,420,350,"Rivera Sanchez Maritza Berenice");
texto_alumnos(&surface_pause,420,370,"Peraza Gonzalez Ricardo Neftali");
	SDL_Flip(surface_pause);
        texto(&surface_pause,300,200,"Puntaje");
texto_puntaje(&surface_pause);
SDL_Flip(surface_pause);
SDL_FreeSurface(surface_pause);
return 1;

	}


    
}

int Juego(Tetris* T){
    SDL_Event E;
    SDL_EnableKeyRepeat(200,50);
    Mix_Chunk *effect1,  *effect2; // hago efecto de rotacion
    Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,4096);
    effect1=Mix_LoadWAV("puckkyuuuu.wav");
    effect2=Mix_LoadWAV("tukuc.wav");


	// logica del juego movimientos de las piezas y la rotacion pausa y continuar
    while(!T->termino_juego){
	if(E.type==SDL_QUIT){break;}//cerrar ventana del juego
        T->sca = T->scu;
        if (SDL_GetTicks()>=T->nextmove)
            Descenso(T);
        while (SDL_PollEvent(&E)){

		
            if (E.type!=SDL_KEYDOWN)
                continue;
            if (E.key.keysym.sym == SDLK_DOWN && T->nextmove-SDL_GetTicks()>20) 
                T->nextmove = SDL_GetTicks()+20;
            if (E.key.keysym.sym == SDLK_LEFT){
                T->sca.x--;
	        Mix_PlayChannel(-1,effect2,0);
		}
            if (E.key.keysym.sym == SDLK_RIGHT){
                T->sca.x++;
                Mix_PlayChannel(-1,effect2,0);
		}
            if (E.key.keysym.sym == SDLK_UP){
                T->sca.rotacion = (T->sca.rotacion+1)%4;
		Mix_PlayChannel(-1,effect1,0);
		}
	    if (E.key.keysym.sym == SDLK_SPACE){
                T->sca.rotacion = (T->sca.rotacion+1)%4;
		Mix_PlayChannel(-1,effect1,0);
               }
	     if (E.key.keysym.sym == SDLK_p){
 		PAUSE=1;
		printf("este es el usuario   %s",usuario);
		//printf("PAUSA %d \n" , Descenso(T));
		}
		if (E.key.keysym.sym == SDLK_c){
 		PAUSE=0;
		//printf("NO PAUSA %d \n" , Descenso(T));
		}
		 
	
            validacion_basura(T);
        }
        Render(T);
}	

Mix_FreeChunk(effect1);
Mix_CloseAudio();
    return 0;
}


//***********************************************************************************

//ttf



int texto(SDL_Surface **pantalla,int x , int y,const char *mensaje){


 
    SDL_Surface* texto = NULL; //texto
    TTF_Font *fuente = NULL;  //fuente para escribir
    SDL_Color blanco = { 150, 180, 200}; //color
    SDL_Rect coordenadas;
    coordenadas.x=x;
    coordenadas.y=y;

    
    
// Inicializamos SDL_ttf
	if (TTF_Init() < 0) {
		printf("No se pudo iniciar SDL_ttf: %s\n",SDL_GetError());
		return 1;
	}

	atexit(TTF_Quit);


   

    fuente=TTF_OpenFont( "dalila.ttf", 22 ); //colocamos la fuente y su tamaño, la fuente debe estar en el directorio para funcionar

    texto=TTF_RenderText_Solid(fuente, mensaje, blanco); //generamos el texto a mostrar

    SDL_BlitSurface(texto, NULL, *pantalla, &coordenadas);
SDL_Flip(*pantalla);
   
    


// destruimos la fuente de letra
	TTF_CloseFont(fuente);

}



//**********************************************************************************************
int texto_puntaje(SDL_Surface **pantalla){

char c[5];
//SDL_Rect tmp={10,0};




 
SDL_Surface* texto = NULL; //texto
    TTF_Font *fuente = NULL;  //fuente para escribir
    SDL_Color blanco = { 150, 180, 200}; //color
    SDL_Rect coordenadas;
    coordenadas.x=300;
    coordenadas.y=240;




    
// Inicializamos SDL_ttf
	if (TTF_Init() < 0) {
		printf("No se pudo iniciar SDL_ttf: %s\n",SDL_GetError());
		return 1;
	}

	atexit(TTF_Quit);


   

    fuente=TTF_OpenFont( "dalila.ttf", 22 ); //colocamos la fuente y su tamaño, la fuente debe estar en el directorio para funcionar
	//nota, escogi esa letra porque va mas con el tetris, hay varios fonts pero creo que ese cuela bien

  
//puntaje++;
	 sprintf(c, "%d", puntaje);    
  	 texto=TTF_RenderText_Solid(fuente, c, blanco); //generamos el texto a mostrar
	 SDL_BlitSurface(texto, NULL, *pantalla, &coordenadas);
	 SDL_Flip(*pantalla);




// destruimos la fuente de letra
	TTF_CloseFont(fuente);

}

//********************************************************************************************


//**********************************************************************************************
int texto_nivel(SDL_Surface **pantalla){

char c[5];

 
SDL_Surface* texto = NULL; //texto
    TTF_Font *fuente = NULL;  //fuente para escribir
    SDL_Color blanco = { 150, 180, 200}; //color
    SDL_Rect coordenadas;
    coordenadas.x=300;
    coordenadas.y=170;

    
// Inicializamos SDL_ttf
	if (TTF_Init() < 0) {
		printf("No se pudo iniciar SDL_ttf: %s\n",SDL_GetError());
		return 1;
	}

	atexit(TTF_Quit);

    fuente=TTF_OpenFont( "dalila.ttf", 22 ); //colocamos la fuente y su tamaño, la fuente debe estar en el directorio para funcionar
	//nota, escogi esa letra porque va mas con el tetris, hay varios fonts pero creo que ese cuela bien

  
//puntaje++;
 sprintf(c, "%d", nivel);    
  texto=TTF_RenderText_Solid(fuente, c, blanco); //generamos el texto a mostrar
SDL_BlitSurface(texto, NULL, *pantalla, &coordenadas);
SDL_Flip(*pantalla);



// destruimos la fuente de letra
	TTF_CloseFont(fuente);

}

//****************************************************************************************************************
//texto splash

int texto_pizza(SDL_Surface **pantalla,int x , int y,const char *mensaje){


 
    SDL_Surface* texto = NULL; //texto
    TTF_Font *fuente = NULL;  //fuente para escribir
    SDL_Color blanco = { 255, 255, 255}; //color
    SDL_Rect coordenadas;
    coordenadas.x=x;
    coordenadas.y=y;

    
    
// Inicializamos SDL_ttf
	if (TTF_Init() < 0) {
		printf("No se pudo iniciar SDL_ttf: %s\n",SDL_GetError());
		return 1;
	}

	atexit(TTF_Quit);


   

    fuente=TTF_OpenFont( "one.ttf", 32 ); //colocamos la fuente y su tamaño, la fuente debe estar en el directorio para funcionar

    texto=TTF_RenderText_Solid(fuente, mensaje, blanco); //generamos el texto a mostrar

    SDL_BlitSurface(texto, NULL, *pantalla, &coordenadas);
SDL_Flip(*pantalla);
   
    


// destruimos la fuente de letra
	TTF_CloseFont(fuente);

}


//**********************************************************************************************************************

int texto_alumnos(SDL_Surface **pantalla,int x , int y,const char *mensaje){

 
    SDL_Surface* texto = NULL; //texto
    TTF_Font *fuente = NULL;  //fuente para escribir
    SDL_Color blanco = { 150, 180, 200}; //color
    SDL_Rect coordenadas;
    coordenadas.x=x;
    coordenadas.y=y;

    
    
// Inicializamos SDL_ttf
	if (TTF_Init() < 0) {
		printf("No se pudo iniciar SDL_ttf: %s\n",SDL_GetError());
		return 1;
	}

	atexit(TTF_Quit);

    fuente=TTF_OpenFont( "beton.ttf", 15 ); //colocamos la fuente y su tamaño, la fuente debe estar en el directorio para funcionar

    texto=TTF_RenderText_Solid(fuente, mensaje, blanco); //generamos el texto a mostrar

    SDL_BlitSurface(texto, NULL, *pantalla, &coordenadas);
SDL_Flip(*pantalla);
   
    


// destruimos la fuente de letra
	TTF_CloseFont(fuente);

}

//*****************************************************************************************************************

void borrar(SDL_Surface **pantalla,int x ,int y , int alto , int ancho ){
	SDL_Rect cuadrado;
	Uint32 blue ,red;

	cuadrado.x=x;
	cuadrado.y=y;
	
	cuadrado.w=ancho;
	cuadrado.h=alto;
		

	SDL_FillRect(*pantalla,NULL,SDL_MapRGBA((*pantalla)->format,0,0,0,255));
     
	}
//***********************************************************************************************



int main(int argc,char** argv){

//SPLASH

SDL_Surface *surface_splach;
    Uint32 rmask, gmask, bmask, amask;

    

    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;


    surface_splach = SDL_CreateRGBSurface(SDL_SWSURFACE, 600, 600, 32,
                                   rmask, gmask, bmask, amask);


		surface_splach= SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
    if(surface_splach == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
        exit(1);
    }


texto_pizza(&surface_splach,300,150,"TETRIS");
texto_alumnos(&surface_splach,120,260,"INGRESE EL NOMBRE DEL JUGADOR 1 EN CONSOLA ");
texto_alumnos(&surface_splach,120,280,"FORMATO DE JUGADOR ES  EJM. GOKU ");
//texto(&surface_splach,325,190," UCA ");



SDL_Flip(surface_splach);



SDL_Delay(4000);



printf("Ingrese el nombre del JUGADOR \n");
leercadena(usuario, MAX+1);
//printf("este es el usuario   %s",usuario);






//****************************************************************************************
//menu de pantalla
int i;
SDL_Surface *screen, *icon;
	const int width = X_tamanio_pantalla; //Ancho de pantalla
	const int height = Y_tamanio_pantalla;//Alto de pantalla
	screen = SDL_SetVideoMode(width,height,32,SDL_SWSURFACE);
	Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,4096);
	Mix_Music *music;
	music = Mix_LoadMUS("menutheme.mp3"); //Tema del menu
	//music = Mix_LoadMUS("tetris.mp3"); //Tema del menu
	Mix_PlayMusic(music,-1); //repetir indefinidamente
	TTF_Font *font;
	TTF_Init();
	font = TTF_OpenFont("dalila.ttf",25);
	SDL_Color color = {0,0,0}; //Fondo negro
	SDL_Event event;
	Uint32 times;
	int x, y;
	char *mensaje[3] = {"Iniciar", "Puntuacion", "Salir"};
	SDL_Surface *menus[3];
	int selected[3] = {0,0,0};
	SDL_Color color2[2] = {{255,255,255},{255,0,0}}; //Texto Blanco y rojo

	menus[0] = TTF_RenderText_Solid(font,mensaje[0],color2[0]); //Iniciar
	menus[1] = TTF_RenderText_Solid(font,mensaje[1],color2[0]); //Puntuacion
	menus[2] = TTF_RenderText_Solid(font,mensaje[2],color2[0]); //Salir
    SDL_Rect pos[3];
	//clip_rect.w/2 - menus[0]->clip_rect.w/2 lo que hago es calcular la mitad de la pantalla y restarle lo que ocupa
	//el texto del menu para que quede centrado
	pos[0].x = screen->clip_rect.w/2 - menus[0]->clip_rect.w/2;
	pos[0].y = 150;
	pos[1].x = screen->clip_rect.w/2 - menus[1]->clip_rect.w/2;
	pos[1].y = 190;
	pos[2].x = screen->clip_rect.w/2 - menus[2]->clip_rect.w/2;
	pos[2].y = 230;	
	SDL_FillRect(screen,&screen->clip_rect,SDL_MapRGB(screen->format,0x00,0x00,0x00));
	while(1)
         {
                    times = SDL_GetTicks();
                    while(SDL_PollEvent(&event))
                    {
                            switch(event.type)
                            {
                                    case SDL_QUIT:
					//Libera memoria de las imagenes
                                            SDL_FreeSurface(menus[0]);
                                            SDL_FreeSurface(menus[1]);
											SDL_FreeSurface(menus[2]);
                                            return 1;
                                    case SDL_MOUSEMOTION:
				//Con este, segun el valor de la posicion del mouse cambia el color al texto
                                            x = event.motion.x;
                                            y = event.motion.y;
                                            for( i = 0; i < 3; i += 1) {
                                                    if(x>=pos[i].x && x<=pos[i].x+pos[i].w && y>=pos[i].y && y<=pos[i].y+pos[i].h)
                                                    {
                                                            if(!selected[i])
                                                            {
                                                                    selected[i] = 1;
                                                                    SDL_FreeSurface(menus[i]);
                                                                    menus[i] = TTF_RenderText_Solid(font,mensaje[i],color2[1]);
                                                            }
                                                    }
                                                    else
                                                    {
                                                            if(selected[i])
                                                            {
                                                                    selected[i] = 0;
                                                                    SDL_FreeSurface(menus[i]);
                                                                    menus[i] = TTF_RenderText_Solid(font,mensaje[i],color2[0]);

                                                          }
                                                    }
                                            }
                                            break;
                                    case SDL_MOUSEBUTTONDOWN:
                                            x = event.button.x;
                                            y = event.button.y;
                                            for( i = 0; i < 3; i += 1) {
                                                    if(x>=pos[i].x && x<=pos[i].x+pos[i].w && y>=pos[i].y && y<=pos[i].y+pos[i].h)
                                                    {
                                                            SDL_FreeSurface(menus[0]);
                                                            SDL_FreeSurface(menus[1]);
							    SDL_FreeSurface(menus[2]);


if(i==0){


//***************************************************************************************************
//opcion jugar tetris


    Tetris T;
	
    srand((unsigned int)time(NULL)); //para iniciar los numeros aleatorios de la piezas
SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
 SDL_WM_SetCaption("Tetris", "Tetris");
	
    
//Mix_Chunk *effect1;
Mix_Music* music;
Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT	,2,4096);
music=Mix_LoadMUS("tetris.mp3");
//effect1=Mix_LoadWAV("rotacion.wav");
Mix_PlayMusic(music , -1);


// esto es ttf


 SDL_Surface *surface;
    Uint32 rmask, gmask, bmask, amask;

    

    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;


    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 600, 600, 32,
                                   rmask, gmask, bmask, amask);


		surface= SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
    if(surface == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
        exit(1);
    }

//*******************************************************************************************************************
//puntajes 

texto(&surface,300,150,"Nivel");
texto(&surface,300,170,"1");
 SDL_Flip(surface);
    

texto(&surface,300,200,"Puntaje");
texto(&surface,300,240,"0");

texto_pizza(&surface,500,100,"UCA");
texto_alumnos(&surface,420,260,"Desarrolladores: ");
texto_alumnos(&surface,420,290,"Miranda Masin Jose Damian");
texto_alumnos(&surface,420,310,"Ramos Hernandez Marlon David");
texto_alumnos(&surface,420,330,"Perez Avendanio Diego Fernando");
texto_alumnos(&surface,420,350,"Rivera Sanchez Maritza Berenice");
texto_alumnos(&surface,420,370,"Peraza Gonzalez Ricardo Neftali");

//****************************************************************************************************************************
		
		
		tetris_inicio(&T);
   		//pintar_fondo(&T); //cuadros de fondo
    		Juego(&T);
		archivos();
    		tetris__fin(&T);

	
//Mix_FreeChunk(effect1);
Mix_FreeMusic(music);
Mix_CloseAudio();

//************************************************************************************************************

}

//Puntuaciones de todos los mejores jugadores
if(i==1){
Tetris T;

Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,4096);
	Mix_Music *music;
	music = Mix_LoadMUS("puntaje.mp3"); //Tema del menu
	//music = Mix_LoadMUS("tetris.mp3"); //Tema del menu
	Mix_PlayMusic(music,-1); //repetir indefinidamente

SDL_Surface *mostrar_mejores_puntuaciones;
    Uint32 rmask, gmask, bmask, amask;

    

    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;


    mostrar_mejores_puntuaciones= SDL_CreateRGBSurface(SDL_SWSURFACE, 600, 600, 32,
                                   rmask, gmask, bmask, amask);


		mostrar_mejores_puntuaciones= SDL_SetVideoMode(X_tamanio_pantalla,Y_tamanio_pantalla,32,SDL_DOUBLEBUF);
    if(mostrar_mejores_puntuaciones == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
        exit(1);
    }

//codigo de lic oscar catedratico de la uca tomado de ejemplo  de archivos subido al moodle 
FILE *f;
	char cadena[MAX];
	char n1[MAX], n2[MAX], n3[MAX], n4[MAX], n5[MAX];
	int e1, e2,e3,e4,e5;
	char mostrar_p1[16]={0};
	char mostrar_p2[16]={0};
	char mostrar_p3[16]={0};
	char mostrar_p4[16]={0};
	char mostrar_p5[16]={0};
	//char mostrar_p2[MAX];
	
	
	f= fopen("te.txt", "r");
	
	if(f==NULL){
		return 0;
	}
	
	int i;
	char car;
	
	//Limpiando (quedan vacios) todos los arreglos
	limpiarArreglo(n1);
	limpiarArreglo(n2);
	limpiarArreglo(n3);
	limpiarArreglo(n4);
	limpiarArreglo(n5);
		
	while(!feof(f)){
	//Guardando la información del archivo al programa sobre de la primera persona
		//Obtengo los caracteres (leer del archivo letra por letra) uno a uno.
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n1[i]=car;
		}
		//Obtengo el resto de la linea
		fgets(cadena,MAX,f);
		//atoi es una función que transforma una cadena de caracteres númerica a tipo de datos entero "23" -> 23
		e1= atoi(cadena);
		//Limpiando variables para reutilizar
		car = '0';
		limpiarArreglo(cadena);
		
	//Guardando la información de la segunda persona
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n2[i]=car;
		}
		
		fgets(cadena,MAX,f);
		e2= atoi(cadena);
		
		car = '0';
		limpiarArreglo(cadena);
		
	//Guardando la información de la tercera persona 
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n3[i]=car;
		}
		fgets(cadena,MAX,f);
		e3= atoi(cadena);
		
		car = '0';
		limpiarArreglo(cadena);
	
	//Guardando la información de la cuarta persona	
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n4[i]=car;
		}
		fgets(cadena,MAX,f);
		e4= atoi(cadena);

		//Guardando la información de la quinta persona	
		for(i=0;car!='-';i++){
			car= fgetc(f);
			if(car!='-') n5[i]=car;
		}
		fgets(cadena,MAX,f);
		e5= atoi(cadena);


	}
	
	fclose(f);
	
	
sprintf(mostrar_p1,"%d" , e1);
sprintf(mostrar_p2,"%d" , e2);
sprintf(mostrar_p3,"%d" , e3);
sprintf(mostrar_p4,"%d" , e4);
sprintf(mostrar_p5,"%d" , e5);
//sprintf(mostrar_p2,"%d" , e2);
texto(&mostrar_mejores_puntuaciones,300,80,n1);
texto(&mostrar_mejores_puntuaciones,300,100,mostrar_p1);
 SDL_Flip(mostrar_mejores_puntuaciones);
texto(&mostrar_mejores_puntuaciones,300,120,n2);
texto(&mostrar_mejores_puntuaciones,300,140,mostrar_p2);
 SDL_Flip(mostrar_mejores_puntuaciones);
texto(&mostrar_mejores_puntuaciones,300,160,n3);
texto(&mostrar_mejores_puntuaciones,300,180,mostrar_p3);
 SDL_Flip(mostrar_mejores_puntuaciones);
texto(&mostrar_mejores_puntuaciones,300,200,n4);
texto(&mostrar_mejores_puntuaciones,300,220,mostrar_p4);
 SDL_Flip(mostrar_mejores_puntuaciones);


    

SDL_Delay(10000);
tetris__fin(&T);

Mix_FreeMusic(music);
Mix_CloseAudio();
}
                                                           return i;
                                                    }
                                            }
                                            break;
                                    case SDL_KEYDOWN:
                                            if(event.key.keysym.sym == SDLK_ESCAPE)
                                            {
                                                    SDL_FreeSurface(menus[0]);
                                                    SDL_FreeSurface(menus[1]);
						    SDL_FreeSurface(menus[2]);
                                                    return 0;
                                            }
                            }
                    }
                    for( i = 0; i < 3; i += 1) {
                            SDL_BlitSurface(menus[i],NULL,screen,&pos[i]);
                    }
                    SDL_Flip(screen);
                    if(1000/30 > (SDL_GetTicks()-times))
                            SDL_Delay(1000/30 - (SDL_GetTicks()-times));
            }

	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();

//*****************************************************************************************
    return 0;
}
