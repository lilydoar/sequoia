const Mass = @import("mass.zig").Mass;

const Self = @This();

mass: Mass,
pos: [2]f32,
vel: [2]f32,
ang: f32,
angVel: f32,

pub fn integratePos(self: *Self, dt: f32) void {
    self.pos[0] += self.vel[0] * dt;
    self.pos[1] += self.vel[1] * dt;
}

pub fn integrateVel(self: *Self, acc: [2]f32, dt: f32) void {
    // TODO: Mass

    self.vel[0] += acc[0] * dt;
    self.vel[1] += acc[1] * dt;
}
