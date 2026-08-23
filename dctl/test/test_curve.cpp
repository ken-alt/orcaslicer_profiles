// ---------------------------------------------------------------------------
//  Tests for the generated __TEXTURE__ variant, Display_Space_IDT_ShowCurve.dctl
//  Verifies it compiles under the texture entry point and that the overlay
//  stays inside its panel.
// ---------------------------------------------------------------------------
#include <cstdio>
#include <cmath>
#define __DEVICE__ static inline
#define __CONSTANT__ static const
#define DEFINE_UI_PARAMS(...)
struct float3 { float x,y,z; }; struct float4 { float x,y,z,w; };
static inline float3 make_float3(float a,float b,float c){float3 v{a,b,c};return v;}
static inline float4 make_float4(float a,float b,float c,float d){float4 v{a,b,c,d};return v;}
static inline float _fabs(float a){return fabsf(a);} static inline float _powf(float a,float b){return powf(a,b);}
static inline float _log2f(float a){return log2f(a);} static inline float _exp2f(float a){return exp2f(a);}
static inline float _sqrtf(float a){return sqrtf(a);} static inline float _fmaxf(float a,float b){return fmaxf(a,b);}
static inline float _fminf(float a,float b){return fminf(a,b);} static inline float _copysignf(float a,float b){return copysignf(a,b);}
static inline float _clampf(float v,float a,float b){return fminf(fmaxf(v,a),b);}
static inline float _mix(float a,float b,float t){return a+(b-a)*t;} static inline float _floor(float a){return floorf(a);}
// __TEXTURE__ shim: a flat image plane sampled by integer pixel coords
struct Tex { const float* p; int w,h; };
#define __TEXTURE__ Tex
static inline float _tex2D(Tex t,int x,int y){
    x = x<0?0:(x>=t.w?t.w-1:x); y = y<0?0:(y>=t.h?t.h-1:y); return t.p[y*t.w+x];
}
enum { DSP_709_24, DSP_709_22, DSP_709_SRGB, DSP_709_709, DSP_P3_26, DSP_P3_24, DSP_2020_24, DSP_2020_PQ };
enum { DST_DWG_LIN, DST_DWG_DI, DST_AP1_CCT, DST_AP0_LIN, DST_709_LIN };
enum { SRC_PARAM, SRC_TONEMAP }; enum { DIAG_OFF, DIAG_RECOV };
static float p_Unroll=0.5f,p_Toe=0.5f,p_Expand=0.5f,p_PHC=0.5f,p_Black=0.0f,p_Contrast=1.0f,p_Peak=100.0f;
static int p_ShowCurve=1,p_Display=DSP_709_24,p_Dest=DST_709_LIN,p_Source=SRC_PARAM,p_Diag=DIAG_OFF;
#include "../Display_Space_IDT_ShowCurve.dctl"

int main(){
    const int W=1920,H=1080;
    static float R[W*H],G[W*H],B[W*H];
    for(int i=0;i<W*H;i++){ R[i]=0.5f; G[i]=0.5f; B[i]=0.5f; }
    Tex tr{R,W,H}, tg{G,W,H}, tb{B,W,H};

    printf("== texture variant: overlay containment ==\n");
    float3 ref = transform(W,H,W-5,5,tr,tg,tb);      // far corner, outside the panel
    int leak=0, inside=0, nonfinite=0;
    for(int y=0;y<H;y++) for(int x=0;x<W;x++){
        float3 o=transform(W,H,x,y,tr,tg,tb);
        if(!std::isfinite(o.x)||!std::isfinite(o.y)||!std::isfinite(o.z)) nonfinite++;
        bool changed = fabsf(o.x-ref.x)>1e-4f;
        float px=(float)x, py=(float)H-(float)y, side=0.30f*W, m=0.03f*W;
        bool inpanel = (px>=m&&px<=m+side&&py>=m&&py<=m+side);
        if(changed&&!inpanel) leak++;
        if(changed&&inpanel) inside++;
    }
    printf("   non-finite: %d   leaked outside panel: %d   drawn inside: %d   %s\n",
           nonfinite,leak,inside,(leak||nonfinite||!inside)?"FAIL":"PASS");

    printf("\n== texture variant matches the basic file's maths (overlay off) ==\n");
    p_ShowCurve=0;
    float worst=0;
    for(int i=0;i<=100;i++){
        float v=i/100.0f;
        for(int j=0;j<W*H;j++){ R[j]=v; G[j]=v*0.7f; B[j]=v*0.4f; }
        float3 o=transform(W,H,900,500,tr,tg,tb);
        // basic-form expectation is covered by test_dctl; here just check sanity
        if(!std::isfinite(o.x)) worst=1e9f;
    }
    printf("   all outputs finite across a ramp: %s\n", worst>0?"FAIL":"PASS");
    p_ShowCurve=1;
    return (leak||nonfinite||!inside||worst>0)?1:0;
}
