// a single triangle covering the whole screen
static const float3 vertices[3] = {
    float3(-1.0f, -1.0f, 0.0f),
    float3(+3.0f, -1.0f, 0.0f),
    float3(-1.0f, +3.0f, 0.0f),
};

struct VertexOut {
    float4 position : SV_Position;
    float2 uv       : TEXCOORD;
};

struct InverseTransform {
    float4x4 inv_view_proj;
    float3   camera_pos;
    float2   fade_range;
};

ConstantBuffer<InverseTransform> xform;

[shader("vertex")]
VertexOut vsmain(uint vid : SV_VertexID)
{
    VertexOut out;
    out.position = float4(vertices[vid], 1.0);
    out.uv = out.position.xy * 0.5 + 0.5;
    return out;
}

[shader("fragment")]
float4 fsmain(VertexOut input) : SV_Target
{
    // reconstruct ray in world space
    float4 ndc = float4(input.uv * 2.0 - 1.0, 0, 1);
    float4 world = mul(ndc, xform.inv_view_proj);
    world /= world.w;

    float3 ray_dir = normalize(world.xyz - xform.camera_pos);

    // intersect with y = 0 ground
    if (ray_dir.y >= 0) discard;
    float t = abs(xform.camera_pos.y / -ray_dir.y);
    float3 hit = xform.camera_pos + ray_dir * t;

    // procedural grid
    float2 grid_sz = float2(1.0);
    float2 grid_uv = hit.xz / grid_sz;
    float2 grid = abs(frac(grid_uv - 0.5) - 0.5) / fwidth(grid_uv);
    float  line = min(grid.x, grid.y);

    // grid color
    float alpha = exp(-line * line * 0.5);
    float3 color = lerp(float3(0.2, 0.2, 0.2), float3(1, 1, 1), alpha);

    // fade at skyline
    float dist = distance(hit, xform.camera_pos);
    float fade = smoothstep(xform.fade_range.x, xform.fade_range.y, dist);

    return float4(color * fade, 1.0f);
}
