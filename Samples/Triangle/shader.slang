struct VertexInput
{
    float3 position : ATTRIBUTE0;
    float3 color    : ATTRIBUTE1;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color    : COLOR0;
};

ConstantBuffer<float4x4> mvp;

[shader("vertex")]
VertexOutput vsmain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0), mvp); // NOTE: Slang uses HLSL style matrix transform
    output.color = float4(input.color, 1.0);
    return output;
}

[shader("fragment")]
float4 fsmain(VertexOutput input) : SV_Target
{
    return input.color;
}
