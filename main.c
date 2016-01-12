#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define DISPLAY_MIN_WIDTH  32
#define DISPLAY_MIN_HEIGHT 32
#define DISPLAY_MAX_WIDTH  1920
#define DISPLAY_MAX_HEIGHT 1080

static inline long clampl( long value, long vmin, long vmax )
{
	return ( value > vmax ? vmax :
			 value < vmin ? vmin :
			 				value );
}

bool get_next_word( char **at, char *value, const size_t max_read )
{
	// NOTE(troy): This function, iterates through our file data,
	// 			and reads string (a single word seperated by whitespace.)
	// 			true is returned if a value is read, otherwise false

	size_t bytes_read = 0;
	while( *(*at) != '\0' )
	{
		char c = *at[0];
		if ( isdigit(c) || isalpha(c) )
		{
			value[bytes_read] = c;
			++bytes_read;
			++(*at);
		}
		else
		if ( c == '#' )
		{
			while( *at[0] != '\0' && *at[0] != '\r' && *at[0] != '\n' )
			{
				++(*at);
			}
		}
		else
		{
			break;
		}
	}

	if ( bytes_read > 0 )
	{
		value[bytes_read] = '\0';
		return true;
	}
	else
	{
		return false;
	}
}

bool get_next_value( char **at, long *value )
{
	// NOTE(troy): This function, iterates through our file data,
	// 			and reads in the next integer value.
	// 			true is returned if a value is read, otherwise false

	char *next;

	while( *(*at) != '\0' )
	{
		char c = *at[0];
		if ( isdigit(c) )
		{
			*value = strtol( *at, &next, 10 );
			*at = next;
			return true;
		}
		else
		if ( c == '#' )
		{
			while( *at[0] != '\0' && *at[0] != '\r' && *at[0] != '\n' )
			{
				++(*at);
			}
		}
		else
		{
			++(*at);
		}
	}

	return false;
}

void put_pixel( int x, int y )
{
	// NOTE(troy): This function sets a single pixel to
	// 			the screen, at location (x,y).

	glBegin( GL_POINTS );
		glVertex2i( x, y );
	glEnd();
}

int main( int argc, char **argv )
{
	// Check Command Args
	if ( argc < 2 )
	{
		fprintf( stderr, "Error: Please pass input file\n" );
		return 1;
	}

	// Initialise SDL
	if ( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		fprintf( stderr, "Error: Unable to Initialise SDL:\n%s\n", SDL_GetError() );
		return 1;
	}

	// Open Input File
	FILE *file = fopen( argv[1], "rb" );
	if ( file == NULL )
	{
		fprintf( stderr, "Error: Unable to open Input File:\n%s\n", argv[1] );
		return 1;
	}

	// Read Input File to Memory
	fseek( file, 0, SEEK_END );
	size_t file_size = ftell( file );
	fseek( file, 0, SEEK_SET );
	char *input = (char *)malloc( file_size+1 );
	if ( input == NULL )
	{
		fprintf( stderr, "Error: Unable to allocate memory for input: %u bytes\n", file_size+1 );
		return 1;
	}
	fread( input, file_size, 1, file );
	input[file_size] = '\0';
	fclose( file );

	// Declare Iterator and Value for File Data
	char *at = input;
	char val_string[16];
	bool read_result;

	// Check For ASCII Format
	read_result = get_next_word( &at, val_string, 15 );
	if ( !read_result || 
		 ( strcmp(val_string, "P3") != 0 && 
		   strcmp(val_string, "p3") != 0 ) )
	{
		fprintf( stderr, "Error: Format is not in ascii - cannot read!\n" );
		return 1;
	}

	// Create Window
	long s_width = 0;
	long s_height = 0;
	get_next_value( &at, &s_width );
	read_result = get_next_value( &at, &s_height );
	if ( !read_result )
	{
		fprintf( stderr, "Error: Invalid Input; Could not Read Size\n:" );
		return 1;
	}
	if ( s_width < DISPLAY_MIN_WIDTH || s_height < DISPLAY_MIN_HEIGHT )
	{
		fprintf( stderr, "Error: Image too small - min width/height = (%d,%d)\n",
				 DISPLAY_MIN_WIDTH, DISPLAY_MIN_HEIGHT );
		return 1;
	}
	if ( s_width > DISPLAY_MAX_WIDTH || s_height > DISPLAY_MAX_HEIGHT )
	{
		fprintf( stderr, "Error: Image too large - man width/height = (%d,%d)\n",
				 DISPLAY_MAX_WIDTH, DISPLAY_MAX_HEIGHT );
		return 1;
	}
	SDL_Window *window = SDL_CreateWindow(  "PPM Viewer", 
											128, 128, 
											s_width, s_height,
											SDL_WINDOW_OPENGL );
	if ( window == NULL )
	{
		fprintf( stderr, "Error: Could not create SDL Window!\n" );
		return 1;
	}

	// Create OpenGL Context from SDL
	SDL_GLContext context = SDL_GL_CreateContext( window );
	if ( context == 0 )
	{
		fprintf( stderr, "Error: Unable to create GL context for SDL\n" );
		return 1;
	}

	// Read Max Color Value
	long max_color = 255;
	read_result = get_next_value( &at, &max_color );
	if ( !read_result )
	{
		fprintf( stderr, "Error: Could not read max color value" );
		return 1;
	}

	// Init OpenGL
	glClearColor( 1.0f, 1.0f, 1.0f, 0.0f );
	glMatrixMode( GL_PROJECTION );
	gluOrtho2D( 0.0, (GLdouble)s_width, (GLdouble)s_height, 0.0 );

	// Clear Screen
	glClear( GL_COLOR_BUFFER_BIT );

	// Remember this iterator location
	// We will need to reset to it, if we
	// need to redraw the image
	char *pixel_read_start = at;

	// Loop through each RGB triplet and put a
	// pixel on the screen
	long pixel_count = s_width * s_height;
	long pixel_read = 0;
	long r, g, b;
	while( pixel_read < pixel_count )
	{
		if ( !get_next_value(&at, &r) )
			break;
		if ( !get_next_value(&at, &g) )
			break;
		if ( !get_next_value(&at, &b) )
			break;

		r = clampl( r, 0, max_color );
		g = clampl( g, 0, max_color );
		b = clampl( b, 0, max_color );

		glColor3f( (GLfloat)(r)/(GLfloat)(max_color),
				   (GLfloat)(g)/(GLfloat)(max_color),
				   (GLfloat)(b)/(GLfloat)(max_color) );

		put_pixel( pixel_read % s_width, pixel_read / s_width );

		++pixel_read;
	} 

	// Swap Bit Buffer
	SDL_GL_SwapWindow( window );

	// Main Loop 
	bool running = true;
	while( running )
	{
		SDL_Event event;
		while ( SDL_PollEvent( &event ) )
		{
			if ( event.type == SDL_WINDOWEVENT )
			{
				switch( event.window.event )
				{
					// Close Window
					case SDL_WINDOWEVENT_CLOSE: {
						running = false;
					} break;

					// Restored Focus
					case SDL_WINDOWEVENT_RESTORED: {

						// We need to Refresh the Screen

						// Clear the Screen
						glClear( GL_COLOR_BUFFER_BIT );

						// Reset our string iterator to the start
						// of where our pixels are read
						at = pixel_read_start;

						// Loop through each RGB triplet and put a
						// pixel on the screen
						pixel_read = 0;
						while( pixel_read < pixel_count )
						{
							if ( !get_next_value(&at, &r) )
								break;
							if ( !get_next_value(&at, &g) )
								break;
							if ( !get_next_value(&at, &b) )
								break;

							r = clampl( r, 0, max_color );
							g = clampl( g, 0, max_color );
							b = clampl( b, 0, max_color );

							glColor3f( (GLfloat)(r)/(GLfloat)(max_color),
									   (GLfloat)(g)/(GLfloat)(max_color),
									   (GLfloat)(b)/(GLfloat)(max_color) );

							put_pixel( pixel_read % s_width, pixel_read / s_width );

							++pixel_read;
						}

						// Swap Bit Buffer
						SDL_GL_SwapWindow( window );
					} break;
				}
			}
		}
	}

	// NOTE(troy): Our other resources have not been cleared, but for this small
	//			program it should be okay. The OS will clean up the memory, and the 
	//			program will close faster because of it.

	// Free File Memory
	free( input );

	// Return Success
	return 0;
}
