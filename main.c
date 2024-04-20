#include <assert.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#define ARRLEN(arr) (sizeof((arr))/sizeof(*(arr)))
#define MAX_HEIGHT 500
#define MINIMUM_HIT_DISTANCE 0.1

typedef enum ShapeType {
	CIRCLE = 0,
	ELLIPSE,
	RECTANGLE,
} ShapeType;

typedef struct Shape {
	ShapeType type;
	const Vector2 position[3];
	const float size[3];
} Shape;

// https://iquilezles.org/articles/distfunctions/
float distance_to_ellipse(Vector2 p, Shape shape) {
	assert(shape.type == ELLIPSE);

	p = Vector2Subtract(p, shape.position[0]);
	Vector2 r = CLITERAL(Vector2) {shape.size[0], shape.size[1]};
	float k0 = Vector2Length(Vector2Divide(p, r));
	float k1 = Vector2Length(Vector2Divide(p, Vector2Multiply(r, r)));
	return k0 * (k0 - 1.0f) / k1;
}

float distance_to_circle(Vector2 p, Shape shape) {
	assert(shape.type == CIRCLE);

	Vector2 center = shape.position[0];
	float radius = shape.size[0];
	return Vector2Length(Vector2Subtract(center, p)) - radius;
}

Vector2 Vector2Abs(Vector2 v) {
	Vector2 result = { fabs(v.x), fabs(v.y) };

    return result;
}

float distance_to_rect(Vector2 p, Shape shape) {
	assert(shape.type == RECTANGLE);

	Vector2 size = CLITERAL(Vector2) { shape.size[0], shape.size[1] };
	Vector2 center = Vector2Add(*shape.position, Vector2Scale(size, 0.5f));
	Vector2 p1 = Vector2Subtract(p, center);
	Vector2 offset = Vector2Subtract(Vector2Abs(p1), Vector2Scale(size, 0.5f));
	Vector2 q = offset;
	if (q.x < 0) {
		q.x = 0;
	}
	if (q.y < 0) {
		q.y = 0;
	}

	return Vector2Length(q) + fmin(fmax(offset.x, offset.y), 0.0f);
}

float distance_to_shapes(Vector2 p, const Shape *shapes, size_t n) {
	float distance = MAX_HEIGHT;

	for (size_t i = 0; i < n; i++) {
		if (shapes[i].type == CIRCLE) {
			distance = fmin(distance, distance_to_circle(p, shapes[i]));
		}
		else if (shapes[i].type == RECTANGLE) {
			distance = fmin(distance, distance_to_rect(p, shapes[i]));
		}
		else if (shapes[i].type == ELLIPSE) {
			distance = fmin(distance, distance_to_ellipse(p, shapes[i]));
		}
	}

	return distance;
}

void draw_shapes(const Shape *shapes, size_t n) {
	Color color = GetColor(0xa1a1a1ff);
	for (size_t i = 0; i < n; i++) {
		if (shapes[i].type == RECTANGLE) {
			// Vector2 center = Vector2SubtractValue(shapes[i].position, shapes[i].size/2);
			Vector2 size = CLITERAL(Vector2) { shapes[i].size[0], shapes[i].size[1] };
			DrawRectangleV(shapes[i].position[0], size, color);
		}
		else if (shapes[i].type == CIRCLE) {
			DrawCircleV(shapes[i].position[0], shapes[i].size[0], color);
		}
		else if (shapes[i].type == ELLIPSE) {
			DrawEllipse(shapes[i].position[0].x, shapes[i].position[0].y,
					shapes[i].size[0], shapes[i].size[1], color);
		}
	}
}

int main(void) {
	Shape shapes[] = {
		{ .type = RECTANGLE, .position = { 200.0f, 100.0f }, .size = {80.0f, 80.0f} },
		{ .type = CIRCLE, .position = { 250.0f, 250.0f }, .size = {70.0f} },
		{ .type = ELLIPSE, .position = { 50.0f, 250.0f }, .size = {100.0f, 40.0f} },
		{ .type = RECTANGLE, .position = { 400.0f, 250.0f }, .size = {60.0f, 60.0f} },
		{ .type = RECTANGLE, .position = { 600.0f, 400.0f }, .size = {100.0f, 50.0f} },
		// { .type = RECTANGLE, .position = { -1000.0f, 0.0f }, .size = {3000.0f, 10.0f} }, // WALL
	};

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(700, 500, "Ray Marching");
	SetTargetFPS(24);

	bool draw_entire_shape = true;
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(GetColor(0x1e1e1eff));

		if (IsKeyPressed(KEY_SPACE)) {
			draw_entire_shape = !draw_entire_shape;
		}

		if (draw_entire_shape) {
			draw_shapes(shapes, ARRLEN(shapes));
		}

		int max_size = fmax(GetScreenWidth(), GetScreenHeight());
		float step = MINIMUM_HIT_DISTANCE/(max_size/8);

		Vector2 start = GetMousePosition();
		for (float a = -PI; a <= PI; a += step) {
			Vector2 p = start;
			float dist = distance_to_shapes(p, shapes, ARRLEN(shapes));
			if (dist <= EPSILON) {
				continue;
			}
			float total_dist = dist;
			Vector2 ray = Vector2Add(start, CLITERAL(Vector2) { max_size*sinf(a), max_size*cosf(a) });
			while (1) {
				// DrawCircleLinesV(p, dist, WHITE);
				float k = Vector2Length(Vector2Subtract(p, ray)) / dist;
				p = CLITERAL(Vector2) {
					.x = (ray.x - p.x) / k + p.x,
					.y = (ray.y - p.y) / k + p.y,
				};
				// printf("%f\n", dist);
				dist = distance_to_shapes(p, shapes, ARRLEN(shapes));
				total_dist += dist;
				if (dist <= MINIMUM_HIT_DISTANCE) {
					DrawLineV(start, p, GRAY);
					DrawCircleV(p, 1.0f, GREEN);
					break;
				}
				if (total_dist >= max_size) {
					break;
				}
			}
			// DrawLineV(start, p, WHITE);
		}
		EndDrawing();
	}

	CloseWindow();

	return 0;
}