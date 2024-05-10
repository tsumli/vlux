struct ModePushConstants {
    uint mode;
};
layout(push_constant) uniform push_mode { ModePushConstants mode; };
