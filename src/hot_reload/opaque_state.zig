pub const OpaqueState = extern struct {
    ptr: ?*anyopaque,
    size: usize,

    pub fn empty() OpaqueState {
        return OpaqueState{
            .ptr = null,
            .size = 0,
        };
    }

    pub fn toState(self: OpaqueState, T: type) *T {
        return @ptrCast(@alignCast(self.ptr.?));
    }
};
