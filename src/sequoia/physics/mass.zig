const Mass = union(enum) {
    uniform: f32,
    nonuniform: struct {
        mass: f32,
        offset: [2]f32,
        moment_of_interia: f32,
    },
    discrete: struct {
        mass_points: [][2]f32,
        point_masses: []f32,
    },
};
