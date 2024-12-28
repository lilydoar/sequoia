// Collision queries:
// - intersection testing. A boolean test of whether two (static) objects,
// A and B, are overlapping at their given positions and orientations.
// - intersection finding. Involves finding one or more points of contact.
// - contact manifold. The set of contact points.
// - penetration depth. Defined as the minimum translational distance:
// The length of the shortest movement vector that would separate the objects.
// - separation distance. The minimum distance between points in A and points in B.
// - time of impact: The next time of collision
//
// Collision Phases:
// - broad phase. Exclude objects which are definitiely not colliding.
// Objects which may be colliding remain.
// - narrow phase. Determine exact collisions.
//

const std = @import("std");

pub fn Vec2(comptime T: type) type {
    return struct {
        data: [2]T,

        const Self = @This();

        pub fn zero() Self {
            return Self{ .data = [_]T{0} ** 2 };
        }

        pub fn splat(value: T) Self {
            return Self{ .data = [_]T{value} ** 2 };
        }

        pub fn add(self: Self, other: Self) Self {
            return Self{
                .data = .{
                    self.data[0] + other.data[0],
                    self.data[1] + other.data[1],
                },
            };
        }

        pub fn sub(self: Self, other: Self) Self {
            return Self{
                .data = .{
                    self.data[0] - other.data[0],
                    self.data[1] - other.data[1],
                },
            };
        }

        pub fn scale(self: Self, scalar: T) Self {
            return Self{
                .data = .{
                    self.data[0] * scalar,
                    self.data[1] * scalar,
                },
            };
        }

        pub fn dot(self: Self, other: Self) T {
            return self.data[0] * other.data[0] +
                self.data[1] * other.data[1];
        }

        pub fn magSquared(self: Self) T {
            return self.dot(self);
        }

        pub fn magnitude(self: Self) T {
            return @sqrt(self.magSquared());
        }

        pub fn normalize(self: Self) Self {
            const mag = self.magnitude();
            if (mag == 0) return self;
            return self.scale(1 / mag);
        }
    };
}

pub fn AABB(comptime T: type) type {
    return struct {
        center: Vec2(T),
        half_extents: Vec2(T),

        const Self = @This();

        pub fn fromMinMax(min: Vec2(T), max: Vec2(T)) Self {
            return Self{
                .center = .{
                    .data = .{
                        (min.data[0] + max.data[0]) / 2,
                        (min.data[1] + max.data[1]) / 2,
                    },
                },
                .half_extents = .{
                    .data = .{
                        (max.data[0] - min.data[0]) / 2,
                        (max.data[1] - min.data[1]) / 2,
                    },
                },
            };
        }

        pub fn getMin(self: Self) Vec2(T) {
            return Vec2(T){
                .data = .{
                    self.center.data[0] - self.half_extents.data[0],
                    self.center.data[1] - self.half_extents.data[1],
                },
            };
        }

        pub fn getMax(self: Self) Vec2(T) {
            return Vec2(T){
                .data = .{
                    self.center.data[0] + self.half_extents.data[0],
                    self.center.data[1] + self.half_extents.data[1],
                },
            };
        }

        pub fn contains(self: Self, point: Vec2(T)) bool {
            if (@abs(self.center.data[0] - point.data[0]) > self.half_extents.data[0]) return false;
            if (@abs(self.center.data[1] - point.data[1]) > self.half_extents.data[1]) return false;
            return true;
        }

        pub fn test_AABB_AABB(self: Self, other: Self) bool {
            if (@abs(self.center.data[0] - other.center.data[0]) >
                (self.half_extents.data[0] + other.half_extents.data[0])) return false;
            if (@abs(self.center.data[1] - other.center.data[1]) >
                (self.half_extents.data[1] + other.half_extents.data[1])) return false;
            return true;
        }
    };
}

pub fn Circle(comptime T: type) type {
    return struct {
        center: Vec2(T),
        radius: T,

        const Self = @This();

        pub fn test_Circle_Circle(self: Self, other: Self) bool {
            const diff = self.center.sub(other.center);
            const dist_squared = diff.dot(diff);
            const radius_sum = self.radius + other.radius;
            return dist_squared <= radius_sum * radius_sum;
        }

        pub fn contains(self: Self, point: Vec2(T)) bool {
            const diff = self.center.sub(point);
            const dist_squared = diff.dot(diff);
            return dist_squared <= self.radius * self.radius;
        }

        pub fn area(self: Self) T {
            return std.math.pi * self.radius * self.radius;
        }
    };
}

pub fn OBB(comptime T: type) type {
    return struct {
        center: Vec2(T),
        orientation: Vec2(T), // Direction vector for local x-axis
        half_extents: Vec2(T),
    };
}

pub fn Capsule(comptime T: type) type {
    return struct {
        line: [2]Vec2(T),
        radius: T,

        const Self = @This();

        pub fn test_Capsule_Capsule(self: Self, other: Self) bool {
            _ = self;
            _ = other;
            // TODO: Implement closest point between line segments test
            return false;
        }
    };
}

pub fn Lozenge(comptime T: type) type {
    return struct {
        center: Vec2(T),
        half_extents: [2]Vec2(T),
        radius: T,
    };
}

pub fn Slab(comptime T: type) type {
    return struct {
        normal: Vec2(T),
        range: [2]T, // near and far
    };
}
