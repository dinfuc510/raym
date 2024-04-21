#include <assert.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#define ARRLEN(arr) (sizeof((arr))/sizeof(*(arr)))
#define WALL_WIDTH 1000
#define WALL_HEIGHT 600
#define MAX_DISTANCE 4000
#define MINIMUM_HIT_DISTANCE 0.1
#define FRAME_WIDTH 2

typedef enum ShapeType {
	CIRCLE = 0,
	ELLIPSE,
	RECTANGLE,
	TRIANGLE,
} ShapeType;

typedef struct Shape {
	ShapeType type;
	const Vector2 position[3];
	const float size[3];
} Shape;

Vector2 Vector2Abs(Vector2 v) {
    return CLITERAL(Vector2) { fabs(v.x), fabs(v.y) };
}

float sign(float x) {
	return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

Vector2 Vector2MinXY(Vector2 v1, Vector2 v2) {
	return CLITERAL(Vector2) { fmin(v1.x, v2.x), fmin(v1.y, v2.y) };
}

Vector2 Vector2SwapXY(Vector2 v) {
	return CLITERAL(Vector2) { v.y, v.x };
}

// https://iquilezles.org/articles/distfunctions2d/
float distance_to_ellipse(Vector2 p, Shape shape) {
	assert(shape.type == ELLIPSE);

	p = Vector2Subtract(p, shape.position[0]);
	Vector2 ab = CLITERAL(Vector2) { shape.size[0],shape.size[1] };
	p = Vector2Abs(p);
	if(p.x > p.y) {
		p = Vector2SwapXY(p);
		ab = Vector2SwapXY(ab);
	}
	float l = ab.y*ab.y - ab.x*ab.x;
    float m = ab.x*p.x/l;
    float m2 = m*m;
    float n = ab.y*p.y/l;
    float n2 = n*n;
    float c = (m2+n2-1.0)/3.0;
    float c3 = c*c*c;
    float q = c3 + m2*n2*2.0;
    float d = c3 + m2*n2;
    float g = m + m*n2;
    float co = 0.0f;
    if(d < 0.0f) {
        float h = acosf(q/c3)/3.0;
        float s = cosf(h);
        float t = sinf(h)*sqrtf(3.0);
        float rx = sqrtf( -c*(s + t + 2.0) + m2 );
        float ry = sqrtf( -c*(s - t + 2.0) + m2 );
        co = (ry+sign(l)*rx+fabs(g)/(rx*ry)- m)/2.0;
    }
    else {
        float h = 2.0*m*n*sqrtf( d );
        float s = sign(q+h)*pow(fabs(q+h), 1.0/3.0);
        float u = sign(q-h)*pow(fabs(q-h), 1.0/3.0);
        float rx = -s - u - c*4.0 + 2.0*m2;
        float ry = (s - u)*sqrtf(3.0);
        float rm = sqrtf( rx*rx + ry*ry );
        co = (ry/sqrtf(rm-rx)+2.0*g/rm-m)/2.0;
    }
    Vector2 r = Vector2Multiply(ab, CLITERAL(Vector2){co, sqrtf(1.0 - co * co) });
    return Vector2Length(Vector2Subtract(r, p)) * sign(p.y - r.y);
}

float distance_to_circle(Vector2 p, Shape shape) {
	assert(shape.type == CIRCLE);

	Vector2 center = shape.position[0];
	float radius = shape.size[0];
	return Vector2Length(Vector2Subtract(center, p)) - radius;
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

float distance_to_triangle(Vector2 p, Shape shape) {
	Vector2 p0 = shape.position[0], p1 = shape.position[1], p2 = shape.position[2];
	Vector2 e0 = Vector2Subtract(p1,p0), e1 = Vector2Subtract(p2,p1), e2 = Vector2Subtract(p0,p2);
    Vector2 v0 = Vector2Subtract(p, p0), v1 = Vector2Subtract(p, p1), v2 = Vector2Subtract(p, p2);
    Vector2 pq0 = Vector2Subtract(v0, Vector2Scale(e0,Clamp(Vector2DotProduct(v0,e0)/Vector2LengthSqr(e0), 0.0, 1.0)));
    Vector2 pq1 = Vector2Subtract(v1, Vector2Scale(e1,Clamp(Vector2DotProduct(v1,e1)/Vector2LengthSqr(e1), 0.0, 1.0)));
    Vector2 pq2 = Vector2Subtract(v2, Vector2Scale(e2,Clamp(Vector2DotProduct(v2,e2)/Vector2LengthSqr(e2), 0.0, 1.0)));
    float s = sign(e0.x*e2.y - e0.y*e2.x);
    Vector2 d = Vector2MinXY(Vector2MinXY(
    				CLITERAL(Vector2) {Vector2LengthSqr(pq0), s*(v0.x*e0.y-v0.y*e0.x)},
                    CLITERAL(Vector2) {Vector2LengthSqr(pq1), s*(v1.x*e1.y-v1.y*e1.x)}),
                    CLITERAL(Vector2) {Vector2LengthSqr(pq2), s*(v2.x*e2.y-v2.y*e2.x)});
    return -sqrtf(d.x)*sign(d.y);
}

float distance_to_shapes(Vector2 p, const Shape *shapes, size_t n) {
	float distance = MAX_DISTANCE;

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
		else if (shapes[i].type == TRIANGLE) {
			distance = fmin(distance, distance_to_triangle(p, shapes[i]));
		}
	}

	return distance;
}

void draw_plus(Vector2 center, float w, Color color) {
	DrawLineV(
			CLITERAL(Vector2) { center.x - w, center.y },
			CLITERAL(Vector2) { center.x + w, center.y },
	color);
	DrawLineV(
			CLITERAL(Vector2) { center.x, center.y - w },
			CLITERAL(Vector2) { center.x, center.y + w },
	color);
}

void draw_shapes(const Shape *shapes, size_t n) {
	Color color = GetColor(0xa1a1a1ff);
	for (size_t i = 0; i < n; i++) {
		if (shapes[i].type == RECTANGLE) {
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
		else if (shapes[i].type == TRIANGLE) {
			DrawTriangle(shapes[i].position[0], shapes[i].position[1], shapes[i].position[2], color);
		}
	}
}

int main(void) {
	float border = 10.0f;
	Shape shapes[] = {
		{ .type = RECTANGLE, .position = { 200.0f, 100.0f }, .size = {80.0f, 80.0f} },
		{ .type = CIRCLE, .position = { 250.0f, 250.0f }, .size = {70.0f} },
		{ .type = ELLIPSE, .position = { 50.0f, 250.0f }, .size = {100.0f, 40.0f} },
		{ .type = RECTANGLE, .position = { 400.0f, 250.0f }, .size = {60.0f, 60.0f} },
		{ .type = RECTANGLE, .position = { 600.0f, 400.0f }, .size = {100.0f, 50.0f} },
		{ TRIANGLE, {
					CLITERAL(Vector2){100.0f, 400.0f},
					CLITERAL(Vector2){650.0f, 420.0f},
					CLITERAL(Vector2){600.0f, 200.0f},
				  	}},
		{ .type = RECTANGLE, .position = { 0.0f, 0.0f }, .size = { WALL_WIDTH - border, border } }, // WALL
		{ .type = RECTANGLE, .position = { 0.0f, WALL_HEIGHT - border }, .size = { WALL_WIDTH - border, border } }, // WALL
		{ .type = RECTANGLE, .position = { 0.0f, 0.0f }, .size = { border, WALL_HEIGHT - border } }, // WALL
		{ .type = RECTANGLE, .position = { WALL_WIDTH - border, 0.0f }, .size = { border, WALL_HEIGHT - border } }, // WALL
	};
	const float step = PI/180.0f;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(700, 500, "Ray Marching");
	SetTargetFPS(60);

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

		int screen_width = GetScreenWidth(), screen_height = GetScreenHeight();
		Vector2 start = GetMousePosition();
		int max_dist = sqrtf(fmax(Vector2LengthSqr(start), screen_width*screen_width + screen_height*screen_height - Vector2LengthSqr(start)));

		for (float a = -PI; a <= PI; a += step) {
			Vector2 p = start;
			Vector2 end = Vector2Add(start, CLITERAL(Vector2) { max_dist*sinf(a), max_dist*cosf(a) });
			float k = max_dist;
			float dist = distance_to_shapes(p, shapes, ARRLEN(shapes));
			if (dist <= EPSILON) {
				continue;
			}
			float total_dist = dist;
			while (dist > MINIMUM_HIT_DISTANCE && total_dist < max_dist) {
				p = CLITERAL(Vector2) {
					.x = (end.x - p.x) / (k/dist) + p.x,
					.y = (end.y - p.y) / (k/dist) + p.y,
				};
				k -= dist;
				dist = distance_to_shapes(p, shapes, ARRLEN(shapes));
				total_dist += dist;
			}
			if (dist <= MINIMUM_HIT_DISTANCE) {
				// DrawLineV(start, p, GRAY);
				draw_plus(p, FRAME_WIDTH/2, ColorAlpha(GREEN, 1.0f - total_dist/max_dist));
				// if (FRAME_WIDTH/total_dist > step) {
				// 	a += FRAME_WIDTH/total_dist - step;		// FRAME_WIDTH = dist*a0
				// }
			}
			// DrawLineV(start, p, WHITE);
		}
		DrawFPS(0, 0);
		EndDrawing();
	}

	CloseWindow();

	return 0;
}