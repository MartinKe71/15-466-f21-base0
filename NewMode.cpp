#include "NewMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iostream>

NewMode::NewMode() {
	{ //initialize enemies
		enemy_positions.clear();
		float starting_y = player.y + enemy_interval;
		while (starting_y < court_radius.y) {
			add_enemies(enemy_positions, starting_y, 2);
			starting_y += enemy_interval;
		}
		spawn_y = starting_y;
	}
	std::cout << "Score: 0";

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 4 * 3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 4 * 3 + 4 * 1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1, 1);
		std::vector< glm::u8vec4 > data(size.x * size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

NewMode::~NewMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool NewMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {
	if (evt.type == SDL_KEYDOWN) {
		auto keyEvent = evt.key.keysym.sym;
		if (!left_locked && (keyEvent == SDLK_a || keyEvent == SDLK_LEFT)) {
			go_left = true;
			go_right = false;
			left_locked = true;
		}
		else if (!right_locked && (keyEvent == SDLK_d || keyEvent == SDLK_RIGHT)) {
			go_right = true;
			go_left = false;
			right_locked = true;
		}
		else if (!shoot_locked && keyEvent == SDLK_SPACE && bullet_available > 0) {
			player_shoot = true;
			shoot_locked = true;
		}
	}

	if (evt.type == SDL_KEYUP) {
		auto keyEvent = evt.key.keysym.sym;
		if (left_locked && (keyEvent == SDLK_a || keyEvent == SDLK_LEFT)) {
			left_locked = false;
		}
		else if (right_locked && (keyEvent == SDLK_d || keyEvent == SDLK_RIGHT)) {
			right_locked = false;
		}
		else if (shoot_locked && keyEvent == SDLK_SPACE) {
			shoot_locked = false;
		}
	}

	if (evt.type == SDL_MOUSEBUTTONDOWN) {
		auto keyEvent = evt.button.button;
		if (!shoot_locked && keyEvent == SDL_BUTTON_LEFT && bullet_available > 0) {
			player_shoot = true;
			shoot_locked = true;
		}		
	}

	if (evt.type == SDL_MOUSEBUTTONUP) {
		auto keyEvent = evt.button.button;
		if (shoot_locked && keyEvent == SDL_BUTTON_LEFT) {
			shoot_locked = false;
		}
	}

	return false;
}

void NewMode::update(float elapsed) {
	if (game_freeze) return;

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	//---- player update ----
	if (go_right) {
		player.x += 2.0f;
		go_right = false;
	}
	else if (go_left) {
		player.x -= 2.0f;
		go_left = false;
	}

	player.x = std::max(player.x, -court_radius.x + player_radius.x);
	player.x = std::min(player.x, court_radius.x - player_radius.x);

	if (player_shoot ) {
		player_shoot = false;
		if (bullet_available > 0) {
			bullet_available--;
			bullet_used++;
			bullets.emplace_back(player + glm::vec2(0.0f, 0.7f * player_radius.y + 2.0f * bullet_radius.y));
			//std::cout << "Bullets available is" << bullet_available << "\n";
		}
	}
		
	//---- enemy update ----
	assert(time_since_last_movement >= 0.0f);
	time_since_last_movement += elapsed;
	if (time_since_last_movement > movement_interval) {
		time_since_last_movement -= movement_interval;
		uint32_t remove_idx = UINT32_MAX;
		for (uint32_t i = 0; i < enemy_positions.size(); i++) {
			enemy_positions[i].y -= 0.5f;
			if (enemy_positions[i].y < (player.y - player_radius.y * 2.0f)) {
				remove_idx = i;
				//std::cout << "there is one enemy below player" << "\n";
			}
		}

		if (remove_idx != UINT32_MAX) {
			enemy_positions.erase(enemy_positions.begin(), enemy_positions.begin() + remove_idx + 1);
			//std::cout << "erasing enemy" << "\n";

			score += remove_idx + 1;
			if (bullet_available < 5) {
				enemy_survived += remove_idx + 1;
			}
			row_survived++;
			bullet_available = (int)enemy_survived / 5 - bullet_used + 1;
			bullet_available = std::min(5, bullet_available);
			//std::cout << "bullet_available: " << bullet_available << "\n";				
			std::cout << "\r";
			std::cout << "Score: " << score;
			movement_interval = std::max(1.0f / (row_survived / 2.0f + 1.0f), 0.12f);
		}

		auto last_enemy = enemy_positions.back();
		if (last_enemy.y < spawn_y - enemy_interval) {

			int expected_bullet_num = bullet_available + (int) enemy_positions.size() / 5 - std::max((uint32_t)0, three_row_num) - 1;
			//std::cout << "Expected number of bullet is: " << expected_bullet_num << "\n";

			int max_enemy = 2;
			if (expected_bullet_num > 0) {
				if (three_in_a_row < bullet_available) {
					max_enemy = 3;
					three_in_a_row++;
				}
				else {
					max_enemy = 2;
					three_in_a_row = 0;
				}
			}
			//std::cout << "max_enemy is: " << max_enemy << "\n";
			add_enemies(enemy_positions, spawn_y, max_enemy);
		}
	}	

	//---- bullet update ----
	if (!bullets.empty()) {
		for (uint32_t i = 0; i < bullets.size(); i++) {
			bullets[i] += bullet_speed * elapsed;
			if (bullets[i].y > court_radius.y) {
				bullets.erase(bullets.begin() + i);
				i--;
			}
		}
	}

	//---- collision handling ----
	auto rect_a_vs_b = [this](glm::vec2 const& a_pos, glm::vec2 const& a_rad, glm::vec2 const& b_pos, glm::vec2 const& b_rad) {
		//compute area of overlap:
		glm::vec2 min = glm::max(a_pos - a_rad, b_pos - b_rad);
		glm::vec2 max = glm::min(a_pos + a_rad, b_pos + b_rad);

		//if no overlap, no collision:
		if (min.x > max.x || min.y > max.y) return false;
		return true;
	};

	//bullet vs enemy
	if (!bullets.empty()) {
		for (uint32_t i = 0; i < bullets.size(); i++) {
			for (uint32_t j = 0; j < enemy_positions.size(); j++) {
				if (bullets[i].x == enemy_positions[j].x
					&& rect_a_vs_b(bullets[i], bullet_radius, enemy_positions[j], enemy_radius)) {
					enemy_positions.erase(enemy_positions.begin() + j);
					bullets.erase(bullets.begin() + i);
					i--;
					three_row_num--;
					score += 2;
					std::cout << "\r";
					std::cout << "Score: " << score;
					break;
				}
			}
		}
	}

	//enemy vs player
	if (!enemy_positions.empty()) {
		for (uint32_t i = 0; i < enemy_positions.size(); i++) {
			if (enemy_positions[i].x == player.x
				&& rect_a_vs_b(player, player_radius, enemy_positions[i], enemy_radius * 0.8f)) {
				game_freeze = true;
				std::cout << "\n" << "Game Over!" << std::endl;
				return;
			}
		}
	}
}

void NewMode::add_enemies(std::vector< glm::vec2 >& enemy_positions, float pos_y, int max_num)
{
	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	std::uniform_int_distribution<> num_dist(1, max_num);
	int num = num_dist(mt);
	//std::cout << "Number of enemy this row is: " << num << "\n";
	std::uniform_int_distribution<> pos_dist(-1, 1);
	int pos = pos_dist(mt);
	//std::cout << "Randomed position is " << pos << "\n";

	if (num == 3) {
		enemy_positions.emplace_back(glm::vec2(-2.0f, pos_y));
		enemy_positions.emplace_back(glm::vec2(0.0f, pos_y));
		enemy_positions.emplace_back(glm::vec2(2.0f, pos_y));
		three_row_num++;
	}
	else if (num == 2) {
		for (int i = -1; i <= 1; i++) {
			if (i == pos)
				continue;
			else {
				enemy_positions.emplace_back(glm::vec2(i * 2.0f, pos_y));
			}
		}
	}
	else {
		enemy_positions.emplace_back(glm::vec2(pos * 2.0f, pos_y));
	}

}

void NewMode::draw_rectangle(std::vector< Vertex >& vertices, glm::vec2 const& center, glm::vec2 const& radius, glm::u8vec4 const& color) {
	//draw rectangle as two CCW-oriented triangles:
	vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	vertices.emplace_back(glm::vec3(center.x + radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

	vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	vertices.emplace_back(glm::vec3(center.x - radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
}

void NewMode::draw_tank(std::vector< Vertex >& vertices, glm::vec2 const& origin, glm::vec2 const& radius, const std::vector< glm::u8vec4 >& colors) {
	assert(colors.size() == 3);
	
	glm::vec2 base_offset = glm::vec2(0.0f, -0.2f * radius.y);
	glm::vec2 base_radius = glm::vec2(radius.x * 0.6f, radius.y * 0.7f);
	draw_rectangle(vertices, origin + base_offset, base_radius, colors[0]);

	glm::vec2 turret_offset = glm::vec2(0.0f, -0.3f * radius.y);
	glm::vec2 turret_radius = glm::vec2(radius.x * 0.3f, radius.y * 0.3f);
	draw_rectangle(vertices, origin + turret_offset, turret_radius, colors[1]);

	glm::vec2 gun_offset = glm::vec2(0.0f, 0.4f * radius.y);
	glm::vec2 gun_radius = glm::vec2(radius.x * 0.08f, radius.y * 0.5f);
	draw_rectangle(vertices, origin + gun_offset, gun_radius, colors[2]);
}

void NewMode::draw_bullet(std::vector< Vertex >& vertices, glm::vec2 const& origin, glm::vec2 const& radius, const std::vector< glm::u8vec4 >& colors) {
	assert(colors.size() == 2);
	
	// top triangle
	vertices.emplace_back(glm::vec3(origin.x - radius.x, origin.y + radius.y * 0.5f, 0.0f), colors[0], glm::vec2(0.5f, 0.5f));
	vertices.emplace_back(glm::vec3(origin.x + radius.x, origin.y + radius.y * 0.5f, 0.0f), colors[0], glm::vec2(0.5f, 0.5f));
	vertices.emplace_back(glm::vec3(origin.x, origin.y + radius.y, 0.0f), colors[0], glm::vec2(0.5f, 0.5f));

	glm::vec2 body_offset = glm::vec2(0.0f, -radius.y * 0.25f);
	glm::vec2 body_radius = glm::vec2(radius.x, radius.y * 0.75f);
	draw_rectangle(vertices, origin + body_offset, body_radius, colors[1]);
}

void NewMode::draw(glm::uvec2 const& drawable_size) {

	//some nice colors from the course web page:
#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x193b59ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xf2d2b6ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0xf2ad94ff);
	const std::vector< glm::u8vec4 > trail_colors = {
		HEX_TO_U8VEC4(0xf2ad9488),
		HEX_TO_U8VEC4(0xf2897288),
		HEX_TO_U8VEC4(0xbacac088),
	};
#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//shadows for everything (except the trail):

	glm::vec2 s = glm::vec2(0.0f, -shadow_offset);

	draw_rectangle(vertices, glm::vec2(-court_radius.x - wall_radius, 0.0f) + s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(vertices, glm::vec2(court_radius.x + wall_radius, 0.0f) + s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	//draw_rectangle(vertices, glm::vec2(0.0f, -court_radius.y - wall_radius) + s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	//draw_rectangle(vertices, glm::vec2(0.0f, court_radius.y + wall_radius) + s, glm::vec2(court_radius.x, wall_radius), shadow_color);

	//solid objects:

	//walls:
	draw_rectangle(vertices, glm::vec2(-court_radius.x - wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(vertices, glm::vec2(court_radius.x + wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	//draw_rectangle(vertices, glm::vec2(0.0f, -court_radius.y - wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	//draw_rectangle(vertices, glm::vec2(0.0f, court_radius.y + wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

	// enemies
	for (uint32_t i = 0; i < enemy_positions.size(); i++)
	{
		draw_rectangle(vertices, enemy_positions[i], enemy_radius, fg_color);
	}

	// bullets
	std::vector< glm::u8vec4 > bullet_color;
	bullet_color.emplace_back(glm::u8vec4(196.0f, 202.0f, 206.0f, 255.0f));
	bullet_color.emplace_back(glm::u8vec4(184.0f, 115.0f, 51.0f, 255.0f));
	for (uint32_t i = 0; i < bullets.size(); i++) {
		//draw_rectangle(vertices, bullets[i], bullet_radius, fg_color);
		draw_bullet(vertices, bullets[i], bullet_radius, bullet_color);
	}

	if (bullet_available > 0) {
		for (int32_t i = 0; i < bullet_available; i++) {
			glm::vec2 icon_pos = bullet_icon_starting + glm::vec2((i % 5) * -0.8f, 0.0f);
			draw_bullet(vertices, icon_pos, bullet_icon_radius, bullet_color);
		}
	}
	

	//player
	std::vector< glm::u8vec4 > player_color;
	player_color.emplace_back(glm::u8vec4(109.0f, 112.0f, 79.0f, 255.0f));
	player_color.emplace_back(glm::u8vec4(150.0f, 150.0f, 150.0f, 255.0f));
	player_color.emplace_back(glm::u8vec4(150.0f, 150.0f, 150.0f, 255.0f));

	draw_tank(vertices, player, player_radius, player_color);

	//scores:
	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);
	//for (uint32_t i = 0; i < left_score; ++i) {
	//	draw_rectangle(vertices, glm::vec2(-court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	//}
	//for (uint32_t i = 0; i < right_score; ++i) {
	//	draw_rectangle(vertices, glm::vec2(court_radius.x - (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
	//}



	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);


	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}