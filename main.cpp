#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#define M_MATH_IMPLEMENTATION
#include "m_math.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WINDOW_W 600
#define WINDOW_H 600

void close_callback(GLFWwindow * window) {
    printf("close_callback");
}

void size_callback(GLFWwindow * window, int width, int height) {
    printf("size_callback");
}

void cursorpos_callback(GLFWwindow * window, double mx, double my) {}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    printf("key_callback\n");
    glfwSetWindowShouldClose(window, 1);
}

void mousebutton_callback(GLFWwindow * window, int button, int action, int mods) {}

void char_callback(GLFWwindow * window, unsigned int key) {
    printf("char_callback\n");
}

void error_callback(int error, const char* description) {
    printf("%s\n", description);
}

void print_fun(const char* f) {
    printf("%s\n", f);
}

GLuint compile_shader(GLenum type, const char *src) {
    GLuint shader;
    GLint compiled;
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar* strInfoLog = (GLchar*) malloc(infoLogLength * sizeof(char));
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
        printf("Compilation error in shader %s\n", strInfoLog);
        free(strInfoLog);
        glDeleteShader(shader);
        return 0;
    }
    printf("Success!\n");
    return shader;
}

inline float *push_v4_arr(float *v, float x, float y, float z, float a) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = a;
    //log_fmt("push_v4_arr: %f, %f, %f, %f",  x, y, z, a);
    return v + 4;
}

void set_float3(float3 *v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

void set_float2(float2 *v, float x, float y) {
    v->x = x;
    v->y = y;
}

const unsigned char* read_entire_file(char* filename) {
    unsigned char *result = 0;
    
    FILE *file = fopen(filename, "r");
    if(file) {
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        result = (unsigned char *)malloc(file_size + 1);
        fread(result, file_size, 1, file);
        result[file_size] = 0;
        
        fclose(file);
    } else {
    	printf("FAILED TO OPEN %s\n", filename);
    }

    return result;
}

int compile_shader_program(const char* str_vert_shader, const char* str_frag_shader, const char* attrib_name_0, const char* attrib_name_1) {
    GLuint vert_shader;
    GLuint frag_shader;
    GLuint prog_object;

    vert_shader = compile_shader(GL_VERTEX_SHADER, str_vert_shader);
    if(vert_shader == 0) {
        printf("Error compiling vert shader");
        return 1;
    }

    frag_shader = compile_shader(GL_FRAGMENT_SHADER, str_frag_shader);
    if(frag_shader == 0) {
        printf("Error compiling frag shader");
        return 1;
    }

    printf("Creating shader program");

    prog_object = glCreateProgram();
    glAttachShader(prog_object, vert_shader);
    glAttachShader(prog_object, frag_shader);

    if (attrib_name_0 != NULL) {
        printf("Binding attrib 0");
        glBindAttribLocation(prog_object, 0, attrib_name_0);
    }

    if (attrib_name_1 != NULL) {
        printf("Binding attrib 1");
        glBindAttribLocation(prog_object, 1, attrib_name_1);
    }

    printf("Linking shader program");
    glLinkProgram(prog_object);

    return prog_object;
}


int main(int argc, char const *argv[]) {
	printf("Hello!\n");

    GLFWwindow* window;
    if(!glfwInit()) {
        printf("glfw init failed");
    }
    else {
        printf("Glfw initialized");
    }
    window = glfwCreateWindow(WINDOW_W, WINDOW_H, "stb-true-type-demo", NULL, NULL);
    if(!window) {
        printf("Create window failed");
        glfwTerminate();
        return -1;
    }
	
	printf("Setting callbacks");
    glfwSetWindowCloseCallback(window, close_callback);
    glfwSetWindowSizeCallback(window, size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mousebutton_callback);
    glfwSetCharCallback(window, char_callback);
    glfwSetCursorPosCallback(window, cursorpos_callback);
    glfwSetErrorCallback(error_callback);
    printf("Setting context");
    glfwMakeContextCurrent(window);

	char vs_source[] =
        "attribute vec4 position;"
        "uniform mat4 view_matrix;"
        "uniform mat4 projection_matrix;"
        "varying vec2 uvs;"
        "void main(){"
        "uvs = vec2(position.xy);"
        "vec4 p= vec4(position.zw, 0, 1.0);"
        "gl_Position =  projection_matrix * view_matrix * p;"
        "}";

    char fs_source[] =
        "varying vec2 uvs;"
		"uniform sampler2D texture;"
        "void main() {"
        //"gl_FragColor = vec4(1.0,0.0,0.0, 1.0) + texture2D(texture, uvs);"
        "gl_FragColor = texture2D(texture, uvs);"
        "}";
	printf("Compiling shader\n");
    GLuint main_shader = compile_shader_program(vs_source,
    													  fs_source,
    													  "position", "color");


	printf("Setting up camera\n");
    float3 camera_position;
    float3 camera_direction;
    float3 camera_up;

	float view_matrix[] = M_MAT4_IDENTITY();
	float projection_matrix[] = M_MAT4_IDENTITY();
	float model_matrix[] = M_MAT4_IDENTITY();

    float aspect = WINDOW_W / (float)WINDOW_H;
    m_mat4_perspective(projection_matrix, 10.0, aspect, 0.1, 100.0);

	set_float3(&camera_position, 10 ,10, 10);
	set_float3(&camera_direction, 0 - camera_position.x ,0 - camera_position.y , 0 - camera_position.z);
	set_float3(&camera_up, 0,1,0);
	
	m_mat4_lookat(view_matrix, &camera_position, &camera_direction, &camera_up);

	printf("Creating mesh\n");
	int vertex_data_size = sizeof(float) * 6 * 4;
	float* vertex_data = (float *) malloc(vertex_data_size);

	float* buf = vertex_data;
    {
		float scale = 3.0;
		buf = push_v4_arr(buf, 0.0, 1.0, -scale, scale);
		buf = push_v4_arr(buf, 1.0, 0.0, scale, -scale);
		buf = push_v4_arr(buf, 1.0, 1.0, scale, scale);
		buf = push_v4_arr(buf, 0.0, 1.0, -scale, scale);
		buf = push_v4_arr(buf, 0.0, 0.0, -scale, -scale);
		buf = push_v4_arr(buf, 1.0, 0.0, scale, -scale);
    }

    printf("Loading font !!\n");
    const unsigned char* font_file = read_entire_file("Roboto.ttf");
    
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, font_file, 0)) {
        printf("init font failed\n");
        return 1;
    } else {
        printf("font loaded\n");
    }

    int font_size = 70;

    const char font_first_char = ' ';
    const int font_char_count = '~' - ' ';
    const int bitmap_width = 512;
    const int bitmap_height = 256;
    const int oversample_x = 1;
    const int oversample_y = 1;

    printf("font_init - font_first_char: %d\n", font_first_char);
    printf("font_init - font_char_count: %d\n", font_char_count);

    uint8_t* bitmap = (uint8_t *) malloc(
            sizeof(uint8_t) * bitmap_width * bitmap_height);
    stbtt_packedchar* font_char_data = (stbtt_packedchar *) malloc(
            sizeof(stbtt_packedchar) * font_char_count);

	printf("font_init - pack_begin\n");
    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, bitmap, bitmap_width, bitmap_height, 0, 1,
                         NULL)) {
        printf("failed to pack begin font\n");
        assert(0);
    }

    printf("font_init - pack_font_range\n");
    stbtt_PackSetOversampling(&context, oversample_x, oversample_y);
    if (!stbtt_PackFontRange(&context, font_file, 0, font_size, font_first_char,
                             font_char_count, font_char_data)) {
        printf("failed to pack font\n");
        assert(0);
    }
    stbtt_PackEnd(&context);
    printf("font_init - pack_end\n");
	
	glEnable(GL_TEXTURE_2D);	
	printf("font_init - uploading texture\n");

	GLuint font_texture = 0;

    stbi_write_png("font.png", bitmap_width, bitmap_height, 1, bitmap, 0);

    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    delete bitmap;
    delete font_file;

    bool render_a_character = false;

    if (render_a_character) {
    	float x = 0;
    	float y = 0;
		stbtt_aligned_quad q;

    	char character = 'B';

		stbtt_GetPackedQuad(font_char_data,
                                bitmap_width,
                                bitmap_height,
                                character - font_first_char,
                                &x,
                                &y,
                                &q,
                                1);


		float* buf = vertex_data;
	    {
	    	
			float scale = 1.0;
			buf = push_v4_arr(buf, q.s0, q.t0, -scale, scale);
			buf = push_v4_arr(buf, q.s1, q.t0, scale, -scale);
			buf = push_v4_arr(buf, q.s1, q.t1, scale, scale);
			buf = push_v4_arr(buf, q.s0, q.t0, -scale, scale);
			buf = push_v4_arr(buf, q.s1, q.t1, -scale, -scale);
			buf = push_v4_arr(buf, q.s0, q.t1, scale, -scale);
	    }
    }

    printf("Loading test texture\n");
	GLuint test_texture = 0;
	{
		int width, height, channels;
	    stbi_set_flip_vertically_on_load(true);
	    unsigned char *pixels = stbi_load("texture_map.png", &width, &height, &channels, 0);
	    assert(pixels != NULL);

	    glGenTextures(1, &test_texture);
	    glBindTexture(GL_TEXTURE_2D, test_texture);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	    if (channels == 3) {
	        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	    } else {
	        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
	                     pixels);
	    }

	    glBindTexture(GL_TEXTURE_2D, 0);
	    stbi_image_free(pixels);
	}
    
    printf("Entering Render Loop\n");

	glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    while(!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		
        glUseProgram(main_shader);
        {
			
			GLint position = glGetAttribLocation(main_shader, "position");
			GLint texture = glGetAttribLocation(main_shader, "texture_unit");

			glUniformMatrix4fv(glGetUniformLocation(main_shader, "view_matrix"),1,GL_FALSE,view_matrix);
			glUniformMatrix4fv(glGetUniformLocation(main_shader, "projection_matrix"), 1, GL_FALSE, projection_matrix);
			glUniform1f(texture, 0.0);

			//glBindTexture(GL_TEXTURE_2D, font_texture);
			glBindTexture(GL_TEXTURE_2D, test_texture);

			int bytes_per_float = 4;
			int stride = bytes_per_float * (4);

			glEnableVertexAttribArray(position);
			glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, stride, vertex_data);

			glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glUseProgram(0);

    	

		glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwMakeContextCurrent(NULL);
    glfwDestroyWindow(window);

	return 0;
}
