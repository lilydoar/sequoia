#include "cglm/struct/vec2.h"
#include <stddef.h>

enum MassType {
  MASS_UNIFORM,
  MASS_NON_UNIFORM,
  MASS_ROTATING,
  MASS_DISCRETE,
};
typedef struct {
  enum MassType type;
  union {
    struct {
      float mass; // Total mass
    } uniform;
    struct {
      vec2s centerOfMass; // Offset from position to center of mass
      float mass;         // Total mass
    } nonuniform;
    struct {
      float momentOfInertia; // Resistance to rotational acceleration
      vec2s centerOfMass;    // Offset from position to center of mass
      float mass;            // Total mass
    } rotating;
    struct {
      vec2s *massPoints;  // Array of local positions for mass points
      float *pointMasses; // Mass at each point
      size_t numPoints;
      float mass; // Total mass
    } discrete;
  };
} Mass;

typedef vec2s Pos;
typedef vec2s Vel;
typedef vec2s Acc;

typedef struct {
  Mass mass;
  Pos pos;
  Vel vel;
  Acc acc;
} Kinetic;

enum ColliderType {
  SHAPE_CIRCLE,
  SHAPE_RECT,
  SHAPE_POLYGON,
  SHAPE_CAPSULE,
  SHAPE_LINE
};
typedef struct {
  struct {
    vec2s center;
    float radius;
  } circle;
  struct {
    vec2s half_extents; // Half-width and half-height
  } rect;

} Collider;

// Calculate center of mass for discrete points
typedef struct {
  vec2s com;
  float totalMass;
} CalcultateCOMRESULT;
CalcultateCOMRESULT calculateCenterOfMass(vec2s *massPoints, float *pointMasses,
                                          size_t numPoints) {
  vec2s com = glms_vec2_zero();
  float totalMass = 0;

  for (size_t i = 0; i < numPoints; i++) {
    com.x += massPoints[i].x * pointMasses[i];
    com.y += massPoints[i].y * pointMasses[i];
    totalMass += pointMasses[i];
  }

  com.x /= totalMass;
  com.y /= totalMass;

  return (CalcultateCOMRESULT){com, totalMass};
}

// Calculate moment of inertia for discrete points around center of mass
float calculateMomentOfInertia(vec2s *massPoints, float *pointMasses,
                               size_t numPoints, vec2s com) {
  float I = 0;

  for (size_t i = 0; i < numPoints; i++) {
    float rx = massPoints[i].x - com.x;
    float ry = massPoints[i].y - com.y;
    float r_squared = rx * rx + ry * ry;
    I += pointMasses[i] * r_squared;
  }

  return I;
}

// Common shapes with their moment of inertia calculations
float getMomentOfInertia_SolidRectangle(float mass, float width, float height) {
  // I = (1/12) * mass * (width^2 + height^2)
  return (mass * (width * width + height * height)) / 12.0f;
}

float getMomentOfInertia_SolidCircle(float mass, float radius) {
  // I = (1/2) * mass * radius^2
  return (mass * radius * radius) / 2.0f;
}

float getMomentOfInertia_HollowCircle(float mass, float radius) {
  // I = mass * radius^2
  return mass * radius * radius;
}

// Calculate moment of inertia for discrete points around axis
float getMomentOfInertia_DiscretePoints(vec2s *points, float *masses,
                                        int numPoints, vec2s axis) {
  float I = 0.0f;

  for (int i = 0; i < numPoints; i++) {
    // Distance squared from axis of rotation
    float dx = points[i].x - axis.x;
    float dy = points[i].y - axis.y;
    float r_squared = dx * dx + dy * dy;

    // I = Σ(m * r²)
    I += masses[i] * r_squared;
  }

  return I;
}

// Parallel axis theorem implementation
float parallelAxisTheorem(float I_cm, float mass, float distance) {
  // I = I_cm + m * d²
  return I_cm + mass * distance * distance;
}

// Example: Angular acceleration calculation
float getAngularAcceleration(float torque, float momentOfInertia) {
  // τ = I * α
  // α = τ / I
  return torque / momentOfInertia;
}

// Example: Rotational kinetic energy
float getRotationalKineticEnergy(float momentOfInertia, float angularVelocity) {
  // E = (1/2) * I * ω²
  return 0.5f * momentOfInertia * angularVelocity * angularVelocity;
}

/*// Example: Apply force to object based on mass distribution*/
/*void applyForce(DiscreteDistributedObject *obj, vec2s force,*/
/*                vec2s applicationPoint) {*/
/*  vec2s com = calculateCenterOfMass(obj);*/
/*  float I = calculateMomentOfInertia(obj, com);*/
/**/
/*  // Linear acceleration (F = ma)*/
/*  vec2s acceleration = {force.x / obj->totalMass, force.y / obj->totalMass};*/
/**/
/*  // Angular acceleration (τ = Iα)*/
/*  float rx = applicationPoint.x - com.x;*/
/*  float ry = applicationPoint.y - com.y;*/
/*  float torque = rx * force.y - ry * force.x;*/
/*  float angularAcceleration = torque / I;*/
/**/
/*  // Implementation of motion updates would go here...*/
/*}*/

bool check_circle_circle(vec2s p1, vec2s p2, float r1, float r2) {
  // Get vector between centers
  vec2s diff = glms_vec2_sub(p2, p1);

  // Square of minimum distance for collision
  float radii_sum = r1 + r2;
  float min_dist_sq = radii_sum * radii_sum;

  // Compare squared distances (avoiding square root)
  return glms_vec2_norm2(diff) <= min_dist_sq;
}

bool check_aabb_aabb(vec2s p1, vec2s p2, vec2s half_extents1,
                     vec2s half_extents2) {
  // Get absolute distance between centers
  float dx = fabs(p1.x - p2.x);
  float dy = fabs(p1.y - p2.y);

  // Sum of half-extents
  float sum_half_width = half_extents1.x + half_extents2.x;
  float sum_half_height = half_extents1.y + half_extents2.y;

  // Collision occurs when distance is less than sum of half-extents
  // on both axes
  return dx <= sum_half_width && dy <= sum_half_height;
}

vec2s closest_point_on_line(vec2s start, vec2s end, vec2s point) {
  vec2s line_vec = glms_vec2_sub(end, start);
  vec2s point_vec = glms_vec2_sub(point, start);

  // Project point onto line vector
  float t = (point_vec.x * line_vec.x + point_vec.y * line_vec.y) /
            glms_vec2_norm2(line_vec);

  // Clamp to segment
  t = fmax(0.0f, fmin(1.0f, t));

  // Calculate closest point
  return (vec2s){start.x + t * line_vec.x, start.y + t * line_vec.y};
}
