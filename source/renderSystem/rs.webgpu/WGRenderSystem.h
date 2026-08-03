#pragma     once
#include    "graphic/FERenderSystem.h"
#include    "wgpu.h"


namespace   FE
{
    DEFINE_CLASS_UUID(WGRenderSystem,"{08B58233-B5F1-48B7-9FC4-187A0AC977BC}");
    class   WGRenderSystem :public FERenderSystem
    {
        IMPLEMENT_CLASS_REFLECT(WGRenderSystem)
    public:
        WGRenderSystem(FEContext& ctx);

        WGRenderSystem(const WGRenderSystem& other)
            :FERenderSystem(other)
        {}
        virtual ~WGRenderSystem();

public:
        inline  auto    instance() const
        {
            return  _native;
        }
        inline  auto    adapter() const
        {
            return  _adapter;
        }

        /// <summary>
        /// �����Ƿ�����Ч��
        /// </summary>
        virtual bool    isValid() const
        {
            return  _native != nullptr;
        }
        /// <summary>
        /// �������������洴��֮ǰ�����������е�����Ⱦ��غ����ᱨ��
        /// ������Ҫ�ڴ�������ɹ�������ڵ�����Ⱦ��غ���
        /// �ú���������ɹ����������
        /// </summary>
        virtual void    initialize()
        {}
        /// <summary>
        /// �������������������֮ǰ������
        /// </summary>
        virtual void    destroy();
        /// <summary>
        /// ������,����ʵ�ֵ�ԭ����
        /// </summary>
        virtual Handle  native()  const
        {
            return  Handle(_native);
        }
        /// <summary>
        /// ��ȡ�Կ��б�
        /// </summary>
        /// <returns></returns>
        virtual const GPUs&    gpuList()   const   override
        {
            return  _gpus;
        }
        /// <summary>
        /// �����豸
        /// </summary>
        /// <param name="info"></param>
        /// <returns></returns>
        virtual FEResult    create()            override;
        virtual Device       createDevice()      override;
    public:
        void            debug(const char* msg);
    protected:
        GPUs            gpuListImpl() const;

    protected:
        WGPUInstance    _native    =   nullptr;
        mutable WGPUAdapter     _adapter   =   nullptr;
        GPUs            _gpus;
    };
}