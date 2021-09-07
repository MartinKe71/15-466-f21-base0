#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

/*
 * PongMode is a game mode that implements a single-player game of Pong.
 */

struct NewMode : Mode {
	NewMode();
	virtual ~NewMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const& drawable_size) override;

	//----- game state -----
	// enemy
	float time_since_last_movement = 0.0f;
	float movement_interval = 1.0f;
	float enemy_interval = 3.5f;
	float spawn_y;
	std::vector < glm::vec2 > enemy_positions;

	glm::vec2 court_radius = glm::vec2(3.0f, 7.0f);
	glm::vec2 bullet_radius = glm::vec2(0.2f, 0.4f);
	glm::vec2 enemy_radius = glm::vec2(0.6f, 0.6f);
	glm::vec2 player_radius = glm::vec2(1.0f, 1.0f);


	// bullets
	uint32_t three_row_num = 0;
	int32_t three_in_a_row = 0;
	int32_t bullet_available = 1;
	uint32_t bullet_used = 0;
	std::vector < glm::vec2 > bullets;
	glm::vec2 bullet_speed = glm::vec2(0.0f, 5.0f);

	glm::vec2 bullet_icon_radius = glm::vec2(0.3f, 0.8f);
	glm::vec2 bullet_icon_starting = glm::vec2(court_radius.x + 3.8f, -court_radius.y + 1.0f);

	// player
	glm::vec2 player = glm::vec2(0.0f, -court_radius.y + 1.0f);

	uint32_t score = 0;
	uint32_t enemy_survived = 0;
	uint32_t row_survived = 0;


	bool go_right = false;
	bool right_locked = false;
	bool go_left = false;
	bool left_locked = false;
	bool player_shoot = false;
	bool shoot_locked = false;

	bool game_freeze = false;

	//----- pretty gradient trails -----

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const& Position_, glm::u8vec4 const& Color_, glm::vec2 const& TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4 * 3 + 1 * 4 + 4 * 2, "NewMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

	void add_enemies(std::vector< glm::vec2 >& enemy_positions, float pos_y, int max_num);
	void draw_rectangle(std::vector< Vertex >& vertices, glm::vec2 const& center, glm::vec2 const& radius, glm::u8vec4 const& color);
	void draw_tank(std::vector< Vertex >& vertices, glm::vec2 const& origin, glm::vec2 const& radius, const std::vector< glm::u8vec4 >& colors);
	void draw_bullet(std::vector< Vertex >& vertices, glm::vec2 const& origin, glm::vec2 const& radius, const std::vector< glm::u8vec4 >& colors);
};
