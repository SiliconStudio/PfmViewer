// Copyright Silicon Studio K.K. 2023
// author: Vivien Oddou
// BSD License

// John Hable/Jim Hejl filmic tone map
static const float U2_A = 0.22; // "Shoulder strength"
static const float U2_B = 0.3;  // "Linear strength"
static const float U2_C = 0.1;  // "Linear angle"
static const float U2_D = 0.2;  // "Toe strength"
static const float U2_E = 0.01; // "Toe numerator"
static const float U2_F = 0.22; // "Toe denominator"
static const float U2_W = 11.2; // "Linear White Point Value"

inline float Uncharted2Tonemap(float x)
{
    static const float ExposureBias = 2;
    x *= ExposureBias;
	return ((x * (U2_A * x + U2_C * U2_B) + U2_D * U2_E) / (x * (U2_A * x + U2_B) + U2_D * U2_F)) - U2_E / U2_F;
}

inline float LinearToGamma(float linearClr)
{
    return pow(linearClr, 1/2.2);
}

inline float ToneComponent(float comp, float exposure)
{
	comp *= exposure;  // Exposure Adjustment
    float postTone = Uncharted2Tonemap(comp);
    float whiteScale = 1.0f / Uncharted2Tonemap(U2_W);
    return postTone * whiteScale;
}

inline float ToneComponentAndToGamma(float comp, float exposure)
{
	comp = ToneComponent(comp, exposure);
    return LinearToGamma(comp);
}

export void ToSignedRgbAllF32Pixels(uniform float vin[], uniform int8 vout[], uniform int count)
{
    foreach (index = 0 ... count)
    {
        float v = vin[index];
        vout[index] = min(255.f, 255 * v) - 128;
    }
}

export void ToSignedRgbAllF16Pixels(uniform uint16 vin[], uniform int8 vout[], uniform int count)
{
    foreach (index = 0 ... count)
    {
        float v = half_to_float(vin[index]);
        vout[index] = min(255.f, 255 * v) - 128;
    }
}

export void GammaAllF32Pixels(uniform float vin[], uniform int8 vout[], uniform int count)
{
    foreach (index = 0 ... count)
    {
        float v = vin[index];
        vout[index] = min(255.f, 255 * LinearToGamma(v)) - 128;
    }
}

export void GammaAllF16Pixels(uniform uint16 vin[], uniform int8 vout[], uniform int count)
{
    foreach (index = 0 ... count)
    {
        float v = half_to_float(vin[index]);
        vout[index] = min(255.f, 255 * LinearToGamma(v)) - 128;
    }
}

// expose and downtone to SDR
export void ToneAllF32Pixels(uniform float vin[], uniform int8 vout[], uniform int count, uniform float exposure)
{
    foreach (index = 0 ... count)
    {
        float v = vin[index];
        vout[index] = min(255.f, 255 * ToneComponent(v, exposure)) - 128;
    }
}

export void ToneAllF16Pixels(uniform uint16 vin[], uniform int8 vout[], uniform int count, uniform float exposure)
{
    foreach (index = 0 ... count)
    {
        float v = half_to_float(vin[index]);
        vout[index] = min(255.f, 255 * ToneComponent(v, exposure)) - 128;
    }
}

// expose, downtone, and gamma-ify
export void ToneAllF32PixelsAndToGamma(uniform float vin[], uniform int8 vout[], uniform int count, uniform float exposure)
{
    foreach (index = 0 ... count)
    {
        float v = vin[index];
        vout[index] = min(255.f, 255 * ToneComponentAndToGamma(v, exposure)) - 128;
    }
}

export void ToneAllF16PixelsAndToGamma(uniform uint16 vin[], uniform int8 vout[], uniform int count, uniform float exposure)
{
    foreach (index = 0 ... count)
    {
        float v = half_to_float(vin[index]);
        vout[index] = min(255.f, 255 * ToneComponentAndToGamma(v, exposure)) - 128;
    }
}

export void GetMinMaxF32(uniform float vin[], uniform int count, uniform float& minOut, uniform float& maxOut)
{
    varying float varMin = 1e34f;
    varying float varMax = -1e34f;
    foreach (index = 0 ... count)
    {
        float v = vin[index];
        if (v < varMin) varMin = v;
        if (v > varMax) varMax = v;
    }
    minOut = reduce_min(varMin);
    maxOut = reduce_max(varMax);
}

export void GetMinMaxF16(uniform uint16 vin[], uniform int count, uniform float& minOut, uniform float& maxOut)
{
    varying float varMin = 1e34f;
    varying float varMax = -1e34f;
    foreach (index = 0 ... count)
    {
        float v = half_to_float(vin[index]);
        if (v < varMin) varMin = v;
        if (v > varMax) varMax = v;
    }
    minOut = reduce_min(varMin);
    maxOut = reduce_max(varMax);
}
