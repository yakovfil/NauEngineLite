// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <d3d12.h>
#include <dxil/dxcapi.h>
#include <wrl/client.h>

#include <fstream>
#include <string>

#include "../../../platform_app/src/platform/windows/windows_window_impl.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/3d/dag_texMgr.h"
#include "nau/app/core_window_manager.h"
#include "nau/app/platform_window.h"
#include "nau/diag/logging.h"
#include "nau/image/dag_texPixel.h"
#include "nau/io/file_system.h"
#include "nau/memory/heap_byte_allocator.h"
#include "nau/module/module_manager.h"
#include "nau/osApiWrappers/dag_cpuJobs.h"
#include "nau/runtime/disposable.h"
#include "nau/service/service_provider.h"
#include "nau/threading/event.h"
#include "nau/threading/set_thread_name.h"
#include "nau/utils/span.h"

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;


const char vertexShader[] =
    "struct VS_IN {\n"
    "  float4 pos : POSITION;\n"
    "  float4 col : COLOR0;\n"
    "};\n\n"
    "struct PS_IN {\n"
    "  float4 pos : SV_POSITION;\n"
    "  float4 col : COLOR;\n"
    "  float2 tex : TEXCOORD;\n"
    "};\n\n"
    "float4 constCol : register(c0); \n\n"
    "PS_IN VSMain(VS_IN input)\n"
    "{\n"
    "  PS_IN output = (PS_IN)0;\n"
    "  output.pos = input.pos;\n"
    "  output.col = constCol;\n //input.col;\n"
    "  output.tex = input.pos.xy*0.5f + float2(0.5f, 0.5f);\n"
    "  return output;\n"
    "}\n\n";


const char pixelShader[] =
"struct PS_IN {\n"
"  float4 pos : SV_POSITION; \n"
"  float4 col : COLOR;       \n"
"  float2 tex : TEXCOORD;    \n"
"};\n\n"
"Texture2D tex : register(t0);\n"
"SamplerState sampl : register(s0);\n"
"float4 PSMain(PS_IN input) : SV_TARGET0\n"
"{\n"
"  float4 col = tex.Sample(sampl, input.tex);\n"
"  return col;\n"
"}\n\n";


const char computeShader[] =
"Buffer<float> buf0 : register(t0);\n"
"Buffer<float> buf1 : register(t1);\n"
"RWBuffer<float> res : register(u0);\n"
"#define BLOCK_SIZE 8\n"
"[numthreads(BLOCK_SIZE, 1, 1)]\n"
"void CSMain( \n"
"    uint3 groupID : SV_GroupID, \n"
"    uint3 groupThreadID : SV_GroupThreadID, \n"
"    uint3 dispatchThreadID : SV_DispatchThreadID, \n"
"    uint  groupIndex : SV_GroupIndex \n"
") \n"
"{\n"
"    res[groupThreadID.x] = buf0[groupThreadID.x] + buf1[groupThreadID.x]; \n"
"}\n\n";


const char vertexShaderForGeometry[] =
    "struct VS_IN {\n"
    "  float4 pos : POSITION;\n"
    "  float4 col : COLOR0;\n"
    "};\n\n"
    "struct GS_IN {\n"
    "  float4 pos : POSITION;\n"
    "  float4 col : COLOR;\n"
    "};\n\n"
    "float4 constCol : register(c0); \n\n"
    "GS_IN VSMain(VS_IN input)\n"
    "{\n"
    "    GS_IN output = (GS_IN)0;\n"
    "    output.pos = input.pos;\n"
    "    output.col = input.col;\n"
    "    return output;\n"
    "}\n\n";

const char geometryShader[] =
    "struct GS_IN {\n"
    "    float4 pos : POSITION;\n"
    "    float4 col : COLOR;\n"
    "};\n\n"
    "struct PS_IN {\n"
    "    float4 pos : SV_POSITION;\n"
    "    float4 col : COLOR;\n"
    "    float2 tex : TEXCOORD;\n"
    "};\n\n"
    "[maxvertexcount(4)] \n"
    "void GSMain(point GS_IN inputPoint[1], inout TriangleStream<PS_IN> outputStream) \n"
    "{ \n"
    "    PS_IN p0, p1, p2, p3; \n"
    "    float sz     = 0.1f; \n"
    "    float4 color = inputPoint[0].col; \n"
    "    float4 wvPos = inputPoint[0].pos; \n"
    "    p0.pos = wvPos + float4(sz, sz, 0, 0); \n"
    "    p0.col = color; \n"
    "    p0.tex = float2(1, 1); \n"
    "\n"
    "    p1.pos = wvPos + float4(-sz, sz, 0, 0); \n"
    "    p1.col = color; \n"
    "    p1.tex = float2(0, 1); \n"
    "\n"
    "    p2.pos = wvPos + float4(-sz, -sz, 0, 0); \n"
    "    p2.col = color; \n"
    "    p2.tex = float2(0, 0); \n"
    "\n"
    "    p3.pos = wvPos + float4(sz, -sz, 0, 0); \n"
    "    p3.col = color; \n"
    "    p3.tex = float2(1, 0); \n"
    "\n"
    "    outputStream.Append(p1); \n"
    "    outputStream.Append(p0); \n"
    "    outputStream.Append(p2); \n"
    "    outputStream.Append(p3); \n"
    "} \n";


const char vertexShaderForHull[] =
    "struct VS_IN {\n"
    "  float4 pos : POSITION;\n"
    "  float4 col : COLOR0;\n"
    "};\n\n"
    "struct HS_IN {\n"
    "  float4 pos : POSITION;\n"
    "  float4 col : COLOR0;\n"
    "};\n\n"
    "HS_IN VSMain(VS_IN input)\n"
    "{\n"
    "  HS_IN output = (HS_IN)0;\n"
    "  output.pos = input.pos;\n"
    "  output.col = input.col;\n"
    "  return output;\n"
    "}\n\n";

const char hullShader[] =
    "struct HS_IN {\n"
    "    float4 pos : POSITION;\n"
    "    float4 col : COLOR;\n"
    "};\n\n"
    "struct HS_CONSTANT_DATA_OUTPUT { \n"
    "    float Edges[3] : SV_TessFactor; \n"
    "    float Inside : SV_InsideTessFactor; \n"
    "}; \n"

    "HS_CONSTANT_DATA_OUTPUT ConstantsHS( InputPatch <HS_IN, 3> p) { \n"
    "    HS_CONSTANT_DATA_OUTPUT Out; \n"
    "    Out.Edges[0] = 32; \n"
    "    Out.Edges[1] = 32; \n"
    "    Out.Edges[2] = 32; \n"
    "    Out.Inside = 32; \n"
    "    return Out; \n"
    "} \n\n"
    "[domain(\"tri\")] \n"
    "[partitioning(\"fractional_odd\")] \n"
    "[outputtopology(\"triangle_cw\")] \n"
    "[outputcontrolpoints(3)] \n"
    "[patchconstantfunc(\"ConstantsHS\")] \n"
    "[maxtessfactor(32.0)] \n"
    "HS_IN HSMain( InputPatch<HS_IN, 3> inputPatch, uint uCPID : SV_OutputControlPointID ) { \n"
    "    HS_IN Out; \n"
    "    Out.pos = inputPatch[uCPID].pos; \n"
    "    Out.col = inputPatch[uCPID].col; \n"
    "    return Out; \n"
    "} \n";

const char domainShader[] =
    "struct HS_IN {\n"
    "    float4 pos : POSITION;\n"
    "    float4 col : COLOR;\n"
    "};\n\n"
    "struct PS_IN {\n"
    "    float4 pos : SV_POSITION; \n"
    "    float4 col : COLOR;       \n"
    "};\n\n"
    "struct HS_CONSTANT_DATA_OUTPUT { \n"
    "    float Edges[3] : SV_TessFactor; \n"
    "    float Inside : SV_InsideTessFactor; \n"
    "}; \n\n"
    "[domain(\"tri\")] \n"
    "PS_IN DSMain( HS_CONSTANT_DATA_OUTPUT input, float3 BarycentricCoordinates : SV_DomainLocation, const OutputPatch<HS_IN, 3> TrianglePatch ) \n"
    "{ \n"
    "    PS_IN Out; \n"
    "    Out.pos = \n"
    "        BarycentricCoordinates.x * TrianglePatch[0].pos + \n"
    "        BarycentricCoordinates.y * TrianglePatch[1].pos + \n"
    "        BarycentricCoordinates.z * TrianglePatch[2].pos; \n"
    "    Out.col = \n"
    "        BarycentricCoordinates.x * TrianglePatch[0].col + \n"
    "        BarycentricCoordinates.y * TrianglePatch[1].col + \n"
    "        BarycentricCoordinates.z * TrianglePatch[2].col; \n"
    "    return Out; \n"
    "} \n";

const char pixelShaderForHull[] =
    "struct PS_IN {\n"
    "  float4 pos : SV_POSITION; \n"
    "  float4 col : COLOR;       \n"
    "};\n\n"
    "float4 PSMain(PS_IN input) : SV_TARGET0\n"
    "{\n"
    "    //float4 col = input.col;\n"
    "    float4 col = float4(1.0f, 1.0f, 1.0f, 1.0f);\n"
    "    return col;\n"
    "}\n\n";


ComPtr<IDxcBlob> CompileShader(const char* shaderCode, uint32_t shaderSize, LPCWSTR entryPoint, LPCWSTR target)
{
    ComPtr<IDxcUtils> pUtils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf()));
    ComPtr<IDxcBlobEncoding> pSource;
    pUtils->CreateBlob(shaderCode, shaderSize, CP_UTF8, pSource.GetAddressOf());


    ComPtr<IDxcCompiler3> pCompiler;
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(pCompiler.GetAddressOf()));


    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = pSource->GetBufferPointer();
    sourceBuffer.Size = pSource->GetBufferSize();
    sourceBuffer.Encoding = 0;


    std::vector<LPCWSTR> arguments;
    // -E for the entry point (eg. 'main')
    arguments.push_back(L"-E");
    arguments.push_back(entryPoint);

    // -T for the target profile (eg. 'ps_6_6')
    arguments.push_back(L"-T");
    arguments.push_back(target);

    // Strip reflection data and pdbs (see later)
    //arguments.push_back(L"-Qstrip_debug");
    //arguments.push_back(L"-Qstrip_reflect");
    arguments.push_back(L"-Qembed_debug");

    arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
    arguments.push_back(DXC_ARG_DEBUG); //-Zi

    //for (const std::wstring& define : defines)
    //{
    //    arguments.push_back(L"-D");
    //    arguments.push_back(define.c_str());
    //}


    ComPtr<IDxcResult> pCompileResult;
    HRESULT hr = pCompiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(pCompileResult.GetAddressOf()));

    // Error Handling. Note that this will also include warnings unless disabled.
    ComPtr<IDxcBlobUtf8> pErrors;
    pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);
    if (pErrors && pErrors->GetStringLength() > 0)
    {
        std::cout << (char*)pErrors->GetBufferPointer();
    }

    ComPtr<IDxcBlob> binRes;
    pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(binRes.GetAddressOf()), nullptr);

    return binRes;
}



class MyD3dInitCB : public Driver3dInitCallback
{
public:
  void verifyResolutionSettings(int &ref_scr_wdt, int &ref_scr_hgt, int base_scr_wdt, int base_scr_hgt,
    bool window_mode) const override
  {
    if ((ref_scr_wdt > base_scr_wdt || ref_scr_hgt > base_scr_hgt) && window_mode)// && !dgs_execute_quiet)
    {
    }
    else
      allowResolutionOverlarge = -1;

    if (allowResolutionOverlarge == 1)
    {
      ref_scr_wdt = base_scr_wdt;
      ref_scr_hgt = base_scr_hgt;
    }
  }

  int validateDesc(Driver3dDesc &) const override { return 1; }

  int compareDesc(Driver3dDesc &, Driver3dDesc &) const override { return 0; }

  bool desiredStereoRender() const override
  {
    return false;
  }

  int64_t desiredAdapter() const override 
  {
      return 0;
  }

  RenderSize desiredRendererSize() const override
  {
    RenderSize s = {0, 0};
    return s;
  }

  const char *desiredRendererDeviceExtensions() const override
  {
    return nullptr;
  }

  const char *desiredRendererInstanceExtensions() const override
  {
    return nullptr;
  }

  VersionRange desiredRendererVersionRange() const override
  {
    VersionRange v = {0, 0};
    return v;
  }

  mutable int allowResolutionOverlarge = -1;
};

static MyD3dInitCB cb;



static std::vector<unsigned char> read_binary_file (const std::string filename)
{
    // binary mode is only for switching off newline translation
    std::ifstream file(filename, std::ios::binary);
    file.unsetf(std::ios::skipws);

    std::streampos file_size;
    file.seekg(0, std::ios::end);
    file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> vec;
    vec.reserve(file_size);
    vec.insert(vec.begin(),
               std::istream_iterator<unsigned char>(file),
               std::istream_iterator<unsigned char>());
    return (vec);
}


namespace nau::test
{
    TEST(TestPlatformApp, Test1)
    {
        using namespace nau::async;

        setDefaultServiceProvider(createServiceProvider());

        getServiceProvider().addService(nau::io::createNativeFileSystem("./"));

        diag::setLogger(diag::createLogger());

        auto manager = createModuleManager();
        {
            const std::string modulesList{NAU_MODULES_LIST};
            nau::loadModulesList(modulesList).ignore();
        }

        manager->doModulesPhase(IModuleManager::ModulesPhase::Init);


        ASSERT_TRUE(d3d::init_driver());
        unsigned memSizeKb = d3d::get_dedicated_gpu_memory_size_kb();

        const char* gameName = "render test";
        uint32_t gameVersion = 1;

        d3d::driver_command(DRV3D_COMMAND_SET_APP_INFO, (void *)gameName, (void *)&gameVersion, nullptr);

        d3d::update_window_mode();

        cpujobs::init();

        threading::Event signal;

        std::thread thread([&signal]
                           {
                               threading::setThisThreadName("Render");

                               auto classes = getServiceProvider().findClasses<ICoreWindowManager>();
                               IClassDescriptor::Ptr classDesc = classes.front();

                               nau::Ptr<ICoreWindowManager> app = classDesc->getConstructor()->invokeToPtr(nullptr, {});
                               getServiceProvider().addService(app);

                               app->bindToCurrentThread();

                               signal.set();

                               app->getActiveWindow().setVisible(true);

                               HWND hwnd = app->getActiveWindow().as<IWindowsWindow*>()->getWindowHandle();

                               void* hwndv = hwnd;
                               HINSTANCE hinst = GetModuleHandle(nullptr);
                               const char* title = "render test";
                               const char* wcName = "wcName";
                               int ncmd = 5;

                               main_wnd_f *wndProc = nullptr;

                               d3d::init_video(hinst, wndProc, wcName, ncmd, hwndv, hwndv, nullptr, title, &cb);

                               shaders::RenderState rendState;
                               shaders::DriverRenderStateId drvRendStateId = d3d::create_render_state(rendState);


                               Vectormath::Vector4 positions[3] = {
                                   Vectormath::Vector4(0.7f, 0.7f, 0.5f, 1.0f),
                                   Vectormath::Vector4(-0.7f, 0.7f, 0.5f, 1.0f),
                                   Vectormath::Vector4(0.7f, -0.7f, 0.5f, 1.0f)
                               };

                               Vectormath::Vector4 colors[3] = {
                                   Vectormath::Vector4(1.0f, 0.0f, 0.0f, 1.0f),
                                   Vectormath::Vector4(0.0f, 0.0f, 1.0f, 1.0f),
                                   Vectormath::Vector4(0.0f, 1.0f, 0.0f, 1.0f)
                               };
                               Sbuffer* posBuf = d3d::create_vb(48, SBCF_DYNAMIC, u8"myPositionBuf"); // (position_float4) * 3
                               posBuf->updateData(0, 48, positions, 0);

                               Sbuffer* colBuf = d3d::create_vb(48, SBCF_DYNAMIC, u8"myColorBuf"); // (color_float4) * 3
                               colBuf->updateData(0, 48, colors, 0);

#pragma region SimpleVertexPixelShaders
                               ComPtr<IDxcBlob> vsBin = CompileShader(vertexShader, sizeof(vertexShader), L"VSMain", L"vs_6_0");
                               ComPtr<IDxcBlob> psBin = CompileShader(pixelShader,  sizeof(pixelShader),  L"PSMain", L"ps_6_0");

                               VDECL shDecl = -1;
                               VSDTYPE ilDefAry[] = {
                                   VSD_STREAM_PER_VERTEX_DATA(0), VSD_REG(VSDR_POS, VSDT_FLOAT4),
                                   VSD_STREAM_PER_VERTEX_DATA(1), VSD_REG(VSDR_DIFF, VSDT_FLOAT4), VSD_END
                               };
                               shDecl = d3d::create_vdecl(ilDefAry);

                               dxil::ShaderResourceUsageTable vsUsageTable1;
                               vsUsageTable1.bRegisterUseMask = 1ul << 0;
                               dxil::ShaderResourceUsageTable psUsageTable1;
                               psUsageTable1.sRegisterUseMask = 1ul << 0;
                               psUsageTable1.tRegisterUseMask = 1ul << 0;
                               psUsageTable1.bRegisterUseMask = 1ul << 0;
                               VPROG   vs = d3d::create_raw_vertex_shader(make_span(reinterpret_cast<const uint8_t*>(vsBin->GetBufferPointer()), vsBin->GetBufferSize()), vsUsageTable1, shDecl);
                               FSHADER ps = d3d::create_raw_pixel_shader(make_span(reinterpret_cast<const uint8_t*>(psBin->GetBufferPointer()), psBin->GetBufferSize()), psUsageTable1);
                               PROGRAM pr = d3d::create_program(vs, ps, shDecl);

                               nau::HeapByteAllocator texAlloc;
                               TexImage32* genImg = TexImage32::create(800, 800, &texAlloc);

                               auto pixPtr = genImg->getPixels();

                               for (int row = 0; row < 800; ++row)
                               {
                                   for (int col = 0; col < 800; ++col)
                                   {
                                       TexPixel32 pix = {};

                                       bool t0 = col/20 % 2 == 1;
                                       bool t1 = row/20 % 2 == t0;
                                       if (t1)
                                       {
                                           pix.r = 255;
                                       }
                                       else
                                       {
                                           pix.g = 255;
                                       }

                                       pixPtr[row*800 + col] = pix;
                                   }
                               }

                               BaseTexture* tex = d3d::create_tex(genImg, 800, 800, TEXFMT_A32B32G32R32F, 1);

                               void* f32ptr = nullptr;
                               int stride;
                               tex->lockimg(&f32ptr, stride, 0, TEXLOCK_WRITE);

                               nau::math::Vector4* color = reinterpret_cast<nau::math::Vector4*>(f32ptr);
                               for (int row = 0; row < 800; ++row)
                               {
                                   for (int col = 0; col < 800; ++col)
                                   {
                                       bool t0 = col/20 % 2 == 1;
                                       bool t1 = row/20 % 2 == t0;
                                       nau::math::Vector4 clr{};

                                       if (t1)
                                       {
                                           clr = nau::math::Vector4(1.0f, 0.0f, 0.0f, 0.0f);
                                       }
                                       else
                                       {
                                           clr = nau::math::Vector4(0.0f, 0.0f, 1.0f, 0.0f);
                                       }
                                       color[row*800 + col] = clr;
                                   }
                               }

                               tex->unlockimg();

                               d3d::SamplerInfo sampInfo;
                               d3d::SamplerHandle sampler = d3d::create_sampler(sampInfo);

#pragma endregion SimpleVertexPixelShaders

#pragma region ComputeShader
                               ComPtr<IDxcBlob> csBin = CompileShader(computeShader, sizeof(computeShader), L"CSMain", L"cs_6_0");

                               dxil::ShaderResourceUsageTable csTable;
                               csTable.tRegisterUseMask = 3;
                               csTable.uRegisterUseMask = 1;
                               PROGRAM csProg = d3d::create_raw_program_cs(make_span(reinterpret_cast<const uint8_t*>(csBin->GetBufferPointer()), csBin->GetBufferSize()), csTable, CSPreloaded::No);

                               Sbuffer* buf0   = d3d::create_sbuffer(0, 8*sizeof(float), TEXFMT_R32F | SBCF_DYNAMIC | SBCF_BIND_SHADER_RES, 0, u8"first");
                               Sbuffer* buf1   = d3d::create_sbuffer(0, 8*sizeof(float), TEXFMT_R32F | SBCF_DYNAMIC | SBCF_BIND_SHADER_RES, 0, u8"second");
                               Sbuffer* bufRes = d3d::create_sbuffer(0, 8*sizeof(float), TEXFMT_R32F | SBCF_DYNAMIC | SBCF_BIND_UNORDERED, 0, u8"result");

                               float* data;
                               buf0->lock(0, 0, (void**) &data, 0);
                               for(size_t i = 0; i < 8; i++)
                               {
                                   data[i] = (float)i;
                               }
                               buf0->unlock();

                               buf1->lock(0, 0, (void**) &data, 0);
                               for(size_t i = 0; i < 8; i++)
                               {
                                   data[i] = (float)i;
                               }
                               buf1->unlock();
#pragma endregion ComputeShader

#pragma region GeometryShader
                               ComPtr<IDxcBlob> geomVsBin = CompileShader(vertexShaderForGeometry, sizeof(vertexShaderForGeometry), L"VSMain", L"vs_6_0");
                               ComPtr<IDxcBlob> gsBin     = CompileShader(geometryShader, sizeof(geometryShader), L"GSMain", L"gs_6_0");

                               d3d::VertexHullDomainGeometryShadersCreationDesc vgDesc{};
                               vgDesc.vs_byte_code = make_span(reinterpret_cast<const uint8_t*>(geomVsBin->GetBufferPointer()), geomVsBin->GetBufferSize());
                               vgDesc.hs_byte_code = make_span<const uint8_t>(nullptr, 0);
                               vgDesc.ds_byte_code = make_span<const uint8_t>(nullptr, 0);
                               vgDesc.gs_byte_code = make_span(reinterpret_cast<const uint8_t*>(gsBin->GetBufferPointer()), gsBin->GetBufferSize());
                               vgDesc.inputLayout = shDecl;

                               VPROG   geomVsGs = d3d::create_raw_vs_hs_ds_gs(vgDesc);
                               PROGRAM geomPr   = d3d::create_program(geomVsGs, ps, shDecl);

#pragma endregion GeometryShader

#pragma region Tesselation
                               ComPtr<IDxcBlob> vsTes = CompileShader(vertexShaderForHull, sizeof(vertexShaderForHull), L"VSMain", L"vs_6_0");
                               ComPtr<IDxcBlob> hsTes = CompileShader(hullShader, sizeof(hullShader), L"HSMain", L"hs_6_0");
                               ComPtr<IDxcBlob> dsTes = CompileShader(domainShader, sizeof(domainShader), L"DSMain", L"ds_6_0");
                               ComPtr<IDxcBlob> psTes = CompileShader(pixelShaderForHull, sizeof(pixelShaderForHull), L"PSMain", L"ps_6_0");

                               dxil::ShaderResourceUsageTable psUsageTable2;
                               psUsageTable2.bRegisterUseMask = 1ul << 0;
                               FSHADER tesPs = d3d::create_raw_pixel_shader(make_span((const uint8_t*)psTes->GetBufferPointer(), psTes->GetBufferSize()), psUsageTable2);

                               d3d::VertexHullDomainGeometryShadersCreationDesc tesDesc{};
                               tesDesc.vs_byte_code = make_span(reinterpret_cast<const uint8_t*>(vsTes->GetBufferPointer()), vsTes->GetBufferSize());
                               tesDesc.hs_byte_code = make_span(reinterpret_cast<const uint8_t*>(hsTes->GetBufferPointer()), hsTes->GetBufferSize());
                               tesDesc.ds_byte_code = make_span(reinterpret_cast<const uint8_t*>(dsTes->GetBufferPointer()), dsTes->GetBufferSize());
                               tesDesc.gs_byte_code = make_span<const uint8_t>(nullptr, 0);
                               tesDesc.inputLayout = shDecl;
                               tesDesc.primitiveType = 10; // D3D_PRIMITIVE_3_CONTROL_POINT_PATCH

                               VPROG   tesShaders = d3d::create_raw_vs_hs_ds_gs(tesDesc);
                               PROGRAM tesPr      = d3d::create_program(tesShaders, tesPs, shDecl);
#pragma endregion Tesselation


                               while(app->pumpMessageQueue(true))
                               {
                                   d3d::set_render_target();
                                   d3d::clearview(CLEAR_TARGET, 0xaaaa,0,0);
                                   d3d::setwire(false);

                                   d3d::set_render_state(drvRendStateId);

#pragma region SimpleVertexPixelShaders
                                   d3d::set_program(pr);

                                   float color[] = {0.2f, 0.1f, 0.5f, 1.0f};
                                   d3d::set_vs_const(0, color, 1);

                                   d3d::settex(0, tex);
                                   d3d::set_sampler(STAGE_PS, 0, sampler);

                                   d3d::setvsrc_ex(0, posBuf, 0, 16);
                                   d3d::setvsrc_ex(1, colBuf, 0, 16);

                                   d3d::draw(PRIM_TRILIST, 0, 1);
#pragma endregion SimpleVertexPixelShaders

#pragma region GeometryShader
                                   d3d::set_program(geomPr);

                                   d3d::setvsrc_ex(0, posBuf, 0, 16);
                                   d3d::setvsrc_ex(1, colBuf, 0, 16);

                                   d3d::draw(PRIM_POINTLIST, 0, 3);
#pragma endregion GeometryShader

#pragma region Tesselation
                                   d3d::set_program(tesPr);

                                   d3d::setvsrc_ex(0, posBuf, 0, 16);
                                   d3d::setvsrc_ex(1, colBuf, 0, 16);

                                   d3d::setwire(true);
                                   d3d::draw(PRIM_3_CONTROL_POINTS, 0, 1);
#pragma endregion Tesselation

#pragma region ComputeShader
                                   d3d::set_program(csProg);

                                   d3d::set_buffer(STAGE_CS, 0, buf0);
                                   d3d::set_buffer(STAGE_CS, 1, buf1);
                                   d3d::set_rwbuffer(STAGE_CS, 0, bufRes);

                                   d3d::dispatch(1,1,1);
#pragma endregion ComputeShader

                                   d3d::update_screen();
                               }

                           });

        signal.wait();

        thread.join();
    }

}  // namespace nau::test
