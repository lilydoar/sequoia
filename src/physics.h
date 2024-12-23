#ifndef PHYSICS_H
#define PHYSICS_H

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
  float mass;
  union {
    bool uniform;
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
  float inertia = 0;

  for (size_t i = 0; i < numPoints; i++) {
    float rx = massPoints[i].x - com.x;
    float ry = massPoints[i].y - com.y;
    float r_squared = rx * rx + ry * ry;
    inertia += pointMasses[i] * r_squared;
  }

  return inertia;
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
  float inertia = 0.0f;

  for (int i = 0; i < numPoints; i++) {
    // Distance squared from axis of rotation
    float dx = points[i].x - axis.x;
    float dy = points[i].y - axis.y;
    float r_squared = dx * dx + dy * dy;

    // I = Σ(m * r²)
    inertia += masses[i] * r_squared;
  }

  return inertia;
}

// Parallel axis theorem implementation
float parallelAxisTheorem(float i_cm, float mass, float distance) {
  // I = i_cm + m * d²
  return i_cm + mass * distance * distance;
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

typedef struct {
  Mass mass;
  vec2s pos;
  vec2s vel;
  vec2s acc;
} Kinematic;

// Apply force to object based on its mass type
void apply_force(Kinematic *obj, vec2s force) {
  float inv_mass = 1.0f / obj->mass.mass;

  // F = ma, therefore a = F/m
  vec2s acceleration = glms_vec2_scale(force, inv_mass);
  obj->acc = glms_vec2_add(obj->acc, acceleration);
}

vec2s calculate_drag_force(const Kinematic *obj, float drag_coeff) {
  // Drag force = -k * v * |v|
  float vel_mag = glms_vec2_norm(obj->vel);
  return glms_vec2_scale(obj->vel, -drag_coeff * vel_mag);
}

vec2s calculate_friction_force(const Kinematic *obj, float friction_coeff) {
  // Simple friction force opposite to velocity
  if (glms_vec2_norm2(obj->vel) < 0.0001f)
    return glms_vec2_zero();

  vec2s friction_dir = glms_vec2_negate(glms_vec2_normalize(obj->vel));
  float normal_force = 0.0f; // You'd calculate this based on contacts
  normal_force = obj->mass.mass;

  return glms_vec2_scale(friction_dir, friction_coeff * normal_force);
}

// Integration functions
void integrate_velocity(Kinematic *obj, float dt) {
  // v = v₀ + at
  obj->vel = glms_vec2_add(obj->vel, glms_vec2_scale(obj->acc, dt));
}

void integrate_position(Kinematic *obj, float dt) {
  // p = p₀ + vt + ½at²
  obj->pos = glms_vec2_add(obj->pos, glms_vec2_scale(obj->vel, dt));
  obj->pos = glms_vec2_add(obj->pos, glms_vec2_scale(obj->acc, 0.5f * dt * dt));
}

typedef struct {
  Mass mass;
  vec2s pos;
  vec2s vel;
  vec2s acc;
  float ang;
  float angVel;
  float angAcc;
} KinematicAngular;

void apply_force_at_point(KinematicAngular *obj, vec2s force, vec2s point) {
  // First apply linear force
  float inv_mass = 1.0f / obj->mass.mass;
  vec2s acceleration = glms_vec2_scale(force, inv_mass);
  obj->acc = glms_vec2_add(obj->acc, acceleration);

  // Handle rotational force if object has rotational mass
  if (obj->mass.type == MASS_ROTATING) {
    // Get position of center of mass in world space
    vec2s com_pos = glms_vec2_add(obj->pos, obj->mass.rotating.centerOfMass);

    // Vector from center of mass to force application point
    vec2s r = glms_vec2_sub(point, com_pos);

    // Calculate torque (2D cross product)
    float torque = r.x * force.y - r.y * force.x;

    // Convert torque to angular acceleration (τ = Iα)
    float angular_acceleration = torque / obj->mass.rotating.momentOfInertia;

    // Add to current angular acceleration
    obj->angAcc += angular_acceleration;
  }
}

float calculate_angular_drag(const KinematicAngular *obj,
                             float angular_drag_coeff) {
  return -angular_drag_coeff * obj->angVel * fabsf(obj->angVel);
}

float calculate_angular_friction(const KinematicAngular *obj,
                                 float angular_friction_coeff) {
  if (fabsf(obj->angVel) < 0.0001f) {
    return 0.0f;
  }

  float moment = (obj->mass.type == MASS_ROTATING)
                     ? obj->mass.rotating.momentOfInertia
                     : 1.0f;

  return -angular_friction_coeff * moment * (obj->angVel > 0.0f ? 1.0f : -1.0f);
}

void integrate_velocity_angular(KinematicAngular *obj, float dt) {
  // Linear velocity integration
  obj->vel = glms_vec2_add(obj->vel, glms_vec2_scale(obj->acc, dt));

  // Angular velocity integration
  obj->angVel += obj->angAcc * dt;
}

void integrate_angle(KinematicAngular *obj, float dt) {
  obj->ang += obj->angVel * dt + 0.5f * obj->angAcc * dt * dt;
}

enum ColliderShape {
  COLLIDER_SHAPE_CIRCLE,
  COLLIDER_SHAPE_AABB,
};
typedef struct {
  enum ColliderShape shape;
  struct {
    float radius;
  } circle;
  struct {
    vec2s half_extents;
  } AABB;
} Collider;

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
  return (vec2s){{start.x + t * line_vec.x, start.y + t * line_vec.y}};
}

#endif
