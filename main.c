#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#define ARRLEN(arr) (sizeof((arr))/sizeof(*(arr)))
#define MAX_HEIGHT 500

typedef enum ShapeType {
	SQUARE = 0,
	CIRCLE
} ShapeType;

typedef struct Shape {
	ShapeType type;
	Vector2 position;
	float size;
} Shape;

float distance_to_circle(Vector2 p, Shape shape) {
	Vector2 center = shape.position;
	float radius = shape.size;
	return Vector2Length(Vector2Subtract(center, p)) - radius;
}

Vector2 Vector2Abs(Vector2 v) {
	Vector2 result = { fabs(v.x), fabs(v.y) };

    return result;
}

float distance_to_square(Vector2 p, Shape shape) {
	float size = shape.size;
	Vector2 center = Vector2AddValue(shape.position, size/2);
	Vector2 p1 = Vector2Subtract(p, center);
	Vector2 offset = Vector2SubtractValue(Vector2Abs(p1), size/2);
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
		else if (shapes[i].type == SQUARE) {
			distance = fmin(distance, distance_to_square(p, shapes[i]));
		}
	}

	return distance;
}

void draw_shapes(const Shape *shapes, size_t n) {
	Color color = GetColor(0xa1a1a1ff);
	for (size_t i = 0; i < n; i++) {
		if (shapes[i].type == SQUARE) {
			// Vector2 center = Vector2SubtractValue(shapes[i].position, shapes[i].size/2);
			DrawRectangleV(shapes[i].position, CLITERAL(Vector2){ shapes[i].size, shapes[i].size }, color);
		}
		else if (shapes[i].type == CIRCLE) {
			DrawCircleV(shapes[i].position, shapes[i].size, color);
		}
	}
}

int main(void) {
	Shape shapes[] = {
		{ SQUARE, { 200.0f, 100.0f }, 80.0f },
		{ CIRCLE, { 250.0f, 250.0f }, 70.0f },
		{ CIRCLE, { 20.0f, 250.0f }, 40.0f },
		{ SQUARE, { 400.0f, 250.0f }, 60.0f },
	};

	InitWindow(700, 500, "Ray Marching");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(GetColor(0x1e1e1eff));

		draw_shapes(shapes, ARRLEN(shapes));

		Vector2 mouse_pos = GetMousePosition();
		float dist = distance_to_shapes(mouse_pos, shapes, ARRLEN(shapes));
		if (dist > 0.01f && dist < MAX_HEIGHT) {
			DrawCircleLinesV(mouse_pos, dist, WHITE);
		}

		EndDrawing();
	}

	CloseWindow();

	return 0;
}