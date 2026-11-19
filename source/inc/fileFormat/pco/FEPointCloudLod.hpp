#pragma     once

#include    <algorithm>
#include    <atomic>
#include    <cmath>
#include    <condition_variable>
#include    <cstdio>
#include    <cstring>
#include    <deque>
#include    <mutex>
#include    <thread>
#include    <vector>

#include    "../../FEObject.h"
#include    "../../FECamera.hpp"
#include    "../../math/FEFrustum.hpp"
#include    "../../mesh/FEMeshBuilder.hpp"
#include    "../../material/FEMaterialPoint565.hpp"
#include    "../../node/FENode.hpp"
#include    "../../graphic/FEScene.h"
#include    "../../graphic/FEFactoryPointCloud.hpp"
#include    "../../graphic/FEUpdateQueue.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEPointCloudLod,"{6D3B2C41-9E58-4F0A-B7C2-1A9D5E8F3C71}");

    /// <summary>
    /// PCO 点云 LOD 调度器。
    /// - 每帧按视锥 + 屏幕误差选出需要显示的节点；
    /// - 后台线程按优先级读取节点数据，主线程建网格并挂到对应节点；
    /// - 超出内存预算或长时间不可见的节点会被卸载(从渲染工厂摘除)。
    /// 所有引擎/场景访问都发生在主线程的 update() 内，工作线程只做文件读取。
    /// </summary>
    class   FEPointCloudLod :public FEObject
    {
    public:
        enum    NodeState :uint8
        {
            StateEmpty      =   0,
            StateQueued     =   1,
            StateReady      =   2,
        };
        /// <summary>
        /// 节点信息:几何与文件偏移在初始化后只读，其余为运行时状态(主线程)
        /// </summary>
        struct  NodeInfo
        {
            double      min[3]      =   {0.0,0.0,0.0};  /// 世界坐标包围盒
            double      max[3]      =   {0.0,0.0,0.0};
            double      center[3]   =   {0.0,0.0,0.0};
            double      radius      =   0.0;
            float       relMin[3]   =   {0.0f,0.0f,0.0f};   /// 相对根中心(PCO 表内数值)
            float       relMax[3]   =   {0.0f,0.0f,0.0f};
            /// 节点局部缩放:把量化顶点还原为世界单位，已按父节点缩放补偿
            double      localScale[3]   =   {1.0,1.0,1.0};
            uint64      dataOffset  =   0;
            uint32      dataSize    =   0;
            uint32      pointCount  =   0;
            uint32      firstChild  =   0xFFFFFFFFu;
            uint8       childMask   =   0;
            uint8       depth       =   0;
            /// 以下为运行时状态
            NodeState   state       =   StateEmpty;
            bool        dispatched  =   false;
            uint64      lastWanted  =   0;
            Node        node;
            Mesh        mesh;
        };
    protected:
        /// <summary>
        /// 读取请求(按屏幕投影尺寸排序)
        /// </summary>
        struct  LoadRequest
        {
            uint32      index   =   0;
            float       priority=   0.0f;
        };
        /// <summary>
        /// 读取结果(8 字节/点的 GPU 顶点字)
        /// </summary>
        struct  LoadResult
        {
            uint32      index   =   0;
            uints       packed;
            bool        ok      =   false;
        };
    protected:
        /// <summary>
        /// 每帧最多上传的节点数，避免单帧卡顿
        /// </summary>
        static  constexpr   uint32  kUploadsPerFrame     =   2;
        /// <summary>
        /// 同时在途的读取请求上限
        /// </summary>
        static  constexpr   uint32  kMaxInFlight         =   16;
        /// <summary>
        /// 后台结果队列上限，超出则丢弃(需要时会重新请求)
        /// </summary>
        static  constexpr   uint32  kMaxResults          =   16;
        /// <summary>
        /// 已加载节点的内存预算(按节点数据字节数计)
        /// </summary>
        static  constexpr   uint64  kMaxLoadedBytes      =   128ull * 1024ull * 1024ull;
        /// <summary>
        /// 屏幕像素半径超过该值则继续细分到子节点
        /// </summary>
        static  constexpr   real    kRefinePixels        =   64.0;
        /// <summary>
        /// 屏幕像素半径小于该值的节点不显示
        /// </summary>
        static  constexpr   real    kMinVisiblePixels    =   1.5;
        /// <summary>
        /// 不可见多少帧后卸载
        /// </summary>
        static  constexpr   uint64  kUnloadFrames        =   180;
    protected:
        String                              _file;
        /// <summary>
        /// 文件句柄缓存:只在加载线程/后台读取线程使用(两者不并发)，
        /// 首次读取时打开，析构或换源时关闭，避免每次读节点都开关文件
        /// </summary>
        std::FILE*                          _fp             =   nullptr;
        Buffer                              _memory;
        Material                            _material;
        /// <summary>
        /// 点云专用逐节点工厂:作为分发候选显式指定,由本模块持有
        /// </summary>
        RFactory                            _factory;
        std::vector<NodeInfo>               _nodes;
        std::vector<uint8>                  _wanted;
        std::vector<float>                  _priority;
        uint64                              _frame          =   0;
        /// 待提交给场景的节点(帧首统一执行)
        std::vector<uint32>                 _pendingReady;
        std::vector<uint32>                 _pendingRemove;
        bool                                _flushQueued    =   false;
        uint64                              _loadedBytes    =   0;
        /// 后台读取
        std::vector<LoadRequest>            _requests;
        std::deque<LoadResult>              _results;
        std::mutex                          _mutex;
        std::condition_variable             _cv;
        std::thread                         _thread;
        std::atomic<bool>                   _stop           =   false;
    public:
        FEPointCloudLod(FEContext& ctx) :FEObject(ctx)   {}
        virtual ~FEPointCloudLod()
        {
            stop();
            closeFile();
        }
        inline  const   std::vector<NodeInfo>&  nodes() const
        {
            return  _nodes;
        }
        inline  void    setMaterial(const Material& material)
        {
            _material   =   material;
        }
        /// <summary>
        /// 设置数据来源:磁盘文件或内存(资源包)
        /// </summary>
        void    setSource(const String& file,const Buffer& memory)
        {
            closeFile();
            _file       =   file;
            _memory     =   memory;
        }
        /// <summary>
        /// 注册节点(必须按文件内的广度优先顺序调用)
        /// </summary>
        void    addNode(const NodeInfo& info,const Node& node)
        {
            NodeInfo    item    =   info;
            item.node           =   node;
            _nodes.emplace_back(item);
            _wanted.emplace_back(0);
            _priority.emplace_back(0.0f);
        }
        /// <summary>
        /// 同步加载根节点:保证加载完成即有粗略轮廓与有效包围盒(供相机取景)
        /// </summary>
        bool    loadRootSync()
        {
            if (_nodes.empty())
                return  false;
            uints   packed;
            if (!readNode(0,packed) || packed.empty())
                return  false;
            NodeInfo&   n       =   _nodes[0];
            Mesh        mesh    =   FEMeshBuilder::makePointMesh565(_ctx,packed);
            if (mesh == nullptr)
                return  false;
            if (_material == nullptr)
                _material   =   defaultMaterial();
            if (_material == nullptr)
                return  false;
            applyNodeMesh(n,mesh);
            n.state         =   StateReady;
            /// 根节点由宿主(读取器返回值)登记到场景，这里同步标记避免重复挂接
            n.dispatched    =   true;
            _loadedBytes    +=  n.dataSize;
            return  true;
        }
        /// <summary>
        /// 启动后台读取线程
        /// </summary>
        void    start()
        {
            if (_thread.joinable() || _nodes.empty())
                return;
            _stop   =   false;
            _thread =   std::thread(&FEPointCloudLod::workerLoop,this);
        }
        /// <summary>
        /// 停止并回收后台线程(销毁前必须完成，之后不可再调用 update)
        /// </summary>
        void    stop()
        {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _stop   =   true;
                _requests.clear();
                _results.clear();
            }
            _cv.notify_all();
            if (_thread.joinable())
                _thread.join();
        }
        /// <summary>
        /// 关闭缓存的文件句柄(必须保证后台线程已停止)
        /// </summary>
        void    closeFile()
        {
            if (_fp != nullptr)
            {
                std::fclose(_fp);
                _fp =   nullptr;
            }
        }
        /// <summary>
        /// 每帧调度(主线程)
        /// </summary>
        void    update(FEScene& scene)
        {
            if (_nodes.empty())
                return;
            auto    camera  =   scene.camera();
            if (camera == nullptr)
                return;
            ++_frame;
            drain();
            select(*camera);
            enqueue();
            evict();
            /// 工厂的增删标记必须在"下一帧工厂更新"之前写入，
            /// 帧末会被统一清除，所以在帧首(场景 runable)再挂接/摘除节点
            queueFlush(scene);
        }
    protected:
        /// <summary>
        /// 创建默认点材质(与 LAS 读取器一致的距离衰减设置)
        /// </summary>
        Material    defaultMaterial()
        {
            Material    material    =   new FEMaterialPoint565(_ctx);
            const   NodeInfo&   root=   _nodes.front();
            const   double  dx  =   root.max[0] - root.min[0];
            const   double  dy  =   root.max[1] - root.min[1];
            const   double  dz  =   root.max[2] - root.min[2];
            const   double  diag=   std::sqrt(dx * dx + dy * dy + dz * dz);
            if (diag > 0.0)
                material->cast<FEMaterialPoint565>()->setPointSizeScale(float(diag * 1.5));
            return  material;
        }
        /// <summary>
        /// 把网格挂到节点:局部缩放由节点相对包围盒决定(与转换工具量化一致)
        /// </summary>
        void    applyNodeMesh(NodeInfo& n,Mesh& mesh)
        {
            if (n.node == nullptr)
                return;
            n.node->setLocalScaling(real3(n.localScale[0],n.localScale[1],n.localScale[2]));
            n.node->setMesh(mesh.get());
            n.node->setMaterial(_material.get());
            /// 必须立即刷新:工厂按节点的全局变换/包围盒填实例数据并做剔除，
            /// 若不更新，新挂上的网格会带着旧变换(null 包围盒)被剔除掉
            n.node->update();
            n.mesh  =   mesh;
            n.state =   StateReady;
        }
        /// <summary>
        /// 从文件/内存读取一个节点的点数据
        /// </summary>
        bool    readNode(uint32 index,uints& packed)
        {
            if (index >= _nodes.size())
                return  false;
            const   NodeInfo&   n   =   _nodes[index];
            packed.clear();
            packed.resize(size_t(n.pointCount) * 2);
            if (n.pointCount == 0)
                return  true;
            if (_memory != nullptr)
            {
                if (n.dataOffset + n.dataSize > _memory->length())
                    return  false;
                std::memcpy(packed.data(),_memory->data() + n.dataOffset,n.dataSize);
                return  true;
            }
            if (_file.empty())
                return  false;
            /// 复用同一个文件句柄:首次读取时打开，之后只 seek/read
            if (_fp == nullptr)
                _fp     =   std::fopen(_file.c_str(),"rb");
            if (_fp == nullptr)
                return  false;
            return  _fseeki64(_fp,int64(n.dataOffset),SEEK_SET) == 0
                &&  std::fread(packed.data(),1,n.dataSize,_fp) == n.dataSize;
        }
        /// <summary>
        /// 子节点下标:children 连续存储，childMask 的位序即槽位顺序
        /// </summary>
        inline  uint32  childIndex(const NodeInfo& n,uint32 bit) const
        {
            uint32  slot    =   0;
            for (uint32 i = 0; i < bit; ++i)
            {
                if ((n.childMask & (1u << i)) != 0)
                    ++slot;
            }
            return  n.firstChild + slot;
        }
        /// <summary>
        /// 是否存在已加载的子孙节点(有则不能直接摘除，避免连带移除)
        /// </summary>
        bool    hasReadyDescendant(uint32 index) const
        {
            std::vector<uint32> stack;
            pushChildren(_nodes[index],stack);
            while (!stack.empty())
            {
                const   uint32  i   =   stack.back();
                stack.pop_back();
                if (i >= _nodes.size())
                    continue;
                if (_nodes[i].state == StateReady)
                    return  true;
                pushChildren(_nodes[i],stack);
            }
            return  false;
        }
        void    pushChildren(const NodeInfo& n,std::vector<uint32>& stack) const
        {
            if (n.childMask == 0)
                return;
            for (uint32 bit = 0; bit < 8; ++bit)
            {
                if ((n.childMask & (1u << bit)) != 0)
                    stack.emplace_back(childIndex(n,bit));
            }
        }
        /// <summary>
        /// 后台线程:按优先级读取节点数据，结果交给主线程
        /// </summary>
        void    workerLoop()
        {
            for (;;)
            {
                LoadRequest req;
                {
                    std::unique_lock<std::mutex>    lock(_mutex);
                    _cv.wait(lock,[this]
                    {
                        return  _stop.load() || !_requests.empty();
                    });
                    if (_stop.load())
                        break;
                    auto    best    =   _requests.begin();
                    for (auto itr = _requests.begin(); itr != _requests.end(); ++itr)
                    {
                        if (itr->priority > best->priority)
                            best    =   itr;
                    }
                    req =   *best;
                    _requests.erase(best);
                }
                LoadResult  result;
                result.index    =   req.index;
                result.ok       =   readNode(req.index,result.packed);
                {
                    /// 结果队列满时等待主线程取走，不能丢弃(否则节点会永远停在 Queued)
                    std::unique_lock<std::mutex>    lock(_mutex);
                    _cv.wait(lock,[this]
                    {
                        return  _stop.load() || _results.size() < kMaxResults;
                    });
                    if (_stop.load())
                        break;
                    _results.emplace_back(std::move(result));
                }
            }
        }
        /// <summary>
        /// 主线程:取出读取结果并挂载网格
        /// </summary>
        void    drain()
        {
            std::deque<LoadResult>  batch;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                while (!_results.empty() && batch.size() < kUploadsPerFrame)
                {
                    batch.emplace_back(std::move(_results.front()));
                    _results.pop_front();
                }
            }
            /// 结果队列腾出空位，唤醒可能等待的后台线程
            _cv.notify_all();
            for (auto& result : batch)
            {
                if (result.index >= _nodes.size())
                    continue;
                NodeInfo&   n   =   _nodes[result.index];
                if (!result.ok || !_wanted[result.index])
                {
                    n.state =   StateEmpty;
                    continue;
                }
                Mesh    mesh    =   FEMeshBuilder::makePointMesh565(_ctx,result.packed);
                if (mesh == nullptr)
                {
                    n.state =   StateEmpty;
                    continue;
                }
                applyNodeMesh(n,mesh);
                n.lastWanted    =   _frame;
                _loadedBytes    +=  n.dataSize;
                _pendingReady.emplace_back(result.index);
            }
        }
        /// <summary>
        /// 视锥 + 屏幕误差选择
        /// </summary>
        void    select(FECamera& camera)
        {
            std::fill(_wanted.begin(),_wanted.end(),uint8(0));
            std::fill(_priority.begin(),_priority.end(),0.0f);
            auto            frustum =   camera.extract();
            const   real3&  eye     =   camera.getEye();
            const   real    viewH   =   std::max<real>(1.0,camera.getViewSize().y);
            /// 距离 1 处的像素/世界单位比例
            const   real    pxScale =   camera.getProject()[1][1] * viewH * 0.5;
            std::vector<uint32> stack;
            stack.emplace_back(0);
            while (!stack.empty())
            {
                const   uint32  index   =   stack.back();
                stack.pop_back();
                if (index >= _nodes.size())
                    continue;
                NodeInfo&       n       =   _nodes[index];
                if (!frustum.cubeInFrustum(n.min[0],n.max[0],n.min[1],n.max[1],n.min[2],n.max[2]))
                    continue;
                const   real3   center(n.center[0],n.center[1],n.center[2]);
                real            dist    =   length(eye - center) - real(n.radius);
                if (dist < 0.05)
                    dist    =   0.05;
                const   real    px      =   real(n.radius) * pxScale / dist;
                if (px < kMinVisiblePixels)
                    continue;
                _wanted[index]      =   1;
                _priority[index]    =   float(px);
                n.lastWanted        =   _frame;
                if (n.childMask != 0 && px > kRefinePixels)
                    pushChildren(n,stack);
            }
        }
        /// <summary>
        /// 主线程:把可见但未加载的节点按优先级投递给工作线程
        /// </summary>
        void    enqueue()
        {
            std::vector<LoadRequest>    candidates;
            for (uint32 i = 0; i < uint32(_nodes.size()); ++i)
            {
                if (_wanted[i] == 0 || _nodes[i].state != StateEmpty)
                    continue;
                candidates.push_back({i,_priority[i]});
            }
            if (candidates.empty())
                return;
            std::sort(candidates.begin(),candidates.end(),[](const LoadRequest& a,const LoadRequest& b)
            {
                return  a.priority > b.priority;
            });
            std::lock_guard<std::mutex> lock(_mutex);
            if (_stop.load())
                return;
            /// 在途 = 待读取请求 + 已读取待上传结果，两者合计受 kMaxInFlight 限制
            const   uint32  inFlight    =   uint32(_requests.size() + _results.size());
            const   uint32  space       =   inFlight < kMaxInFlight
                                            ? uint32(kMaxInFlight - inFlight) : 0;
            for (uint32 i = 0; i < candidates.size() && i < space; ++i)
            {
                NodeInfo&   n   =   _nodes[candidates[i].index];
                if (n.state != StateEmpty)
                    continue;
                n.state         =   StateQueued;
                _requests.emplace_back(candidates[i]);
            }
            _cv.notify_one();
        }
        /// <summary>
        /// 卸载长时间不可见或超出预算的节点
        /// </summary>
        void    evict()
        {
            auto    tryEvict    =   [this](uint32 index) -> bool
            {
                NodeInfo&   n   =   _nodes[index];
                if (n.state != StateReady || hasReadyDescendant(index))
                    return  false;
                /// 网格保留到帧首摘除完成，工厂需要它来定位要移除的实例
                n.state         =   StateEmpty;
                _loadedBytes    -=  n.dataSize;
                _pendingRemove.emplace_back(index);
                return  true;
            };
            /// 1. 长期不可见
            for (uint32 i = 0; i < uint32(_nodes.size()); ++i)
            {
                NodeInfo&   n   =   _nodes[i];
                if (n.state == StateReady && _wanted[i] == 0
                    && _frame - n.lastWanted > kUnloadFrames)
                {
                    tryEvict(i);
                }
            }
            /// 2. 超出内存预算:优先卸载最久未见的节点
            if (_loadedBytes <= kMaxLoadedBytes)
                return;
            std::vector<uint32> candidates;
            for (uint32 i = 0; i < uint32(_nodes.size()); ++i)
            {
                if (_nodes[i].state == StateReady && _wanted[i] == 0)
                    candidates.emplace_back(i);
            }
            std::sort(candidates.begin(),candidates.end(),[this](uint32 a,uint32 b)
            {
                return  _nodes[a].lastWanted < _nodes[b].lastWanted;
            });
            for (size_t i = 0; i < candidates.size(); ++i)
            {
                if (_loadedBytes <= kMaxLoadedBytes * 3 / 4)
                    break;
                tryEvict(candidates[i]);
            }
        }
        /// <summary>
        /// 主线程帧首:把上一帧选出的节点增删提交给场景。
        /// 必须早于工厂更新，否则帧末统一清标记时会被丢弃。
        /// </summary>
        void    flushToScene(FEScene& scene)
        {
            Nodes   removed;
            for (uint32 index : _pendingRemove)
            {
                NodeInfo&   n   =   _nodes[index];
                /// 期间又被选中的节点保留在原工厂里，等新网格覆盖
                if (n.state == StateEmpty && n.mesh != nullptr)
                    removed.emplace_back(n.node);
            }
            if (!removed.empty())
                scene.removeNodesFromSystem(removed);
            for (uint32 index : _pendingRemove)
            {
                NodeInfo&   n   =   _nodes[index];
                if (n.state == StateEmpty && n.mesh != nullptr)
                {
                    n.node->setMesh(nullptr);
                    n.mesh          =   nullptr;
                    n.dispatched    =   false;
                }
            }
            _pendingRemove.clear();

            Nodes   added;
            Nodes   updated;
            for (uint32 index : _pendingReady)
            {
                NodeInfo&   n   =   _nodes[index];
                if (n.state != StateReady || n.mesh == nullptr)
                    continue;
                if (n.dispatched)
                {
                    updated.emplace_back(n.node);
                }
                else
                {
                    added.emplace_back(n.node);
                    n.dispatched    =   true;
                }
            }
            if (!added.empty())
            {
                /// 点云节点几何互不相同、运行期频繁增删:显式指定逐节点工厂作为候选,
                /// 避免批量工厂每次增删都全量重建交错顶点缓冲
                if (_factory == nullptr)
                    _factory    =   new FEFactoryPointCloud(_ctx);
                DispatchOption  option;
                option._reuseExisting   =   true;
                option._factorys.emplace_back(_factory);
                scene.dispatchNodesToSystem(added,nullptr,option);
            }
            if (!updated.empty())
                scene.updateNodesToSystem(updated);
            _pendingReady.clear();
        }
        /// <summary>
        /// 把 flushToScene 投递到下一帧首执行
        /// </summary>
        void    queueFlush(FEScene& scene)
        {
            if (_flushQueued || (_pendingReady.empty() && _pendingRemove.empty()))
                return;
            _flushQueued    =   true;
            FEScene*            target  =   &scene;
            FEPointCloudLod*    self    =   this;
            QueueItems          items;
            QueueItem           item    =   new FEQueueItem(_ctx);
            item->_runable  +=  [target,self]()
            {
                self->_flushQueued  =   false;
                self->flushToScene(*target);
            };
            items.emplace_back(item);
            scene.postRunable(std::move(items));
        }
    public:
        /// <summary>
        /// 把调度器挂到场景更新队列(必须在主线程调用)
        /// </summary>
        static  void    attach(FEScene& scene,SharedPtr<FEPointCloudLod> lod)
        {
            /// 更新回调会长期保存在场景队列中，捕获指针而不是引用
            FEScene*            target  =   &scene;
            FEPointCloudLod*    self    =   lod.get();
            scene.updateQueue().addObject(lod.get(),[target,self](CMDPtr,FEUpdateObject&)
            {
                self->update(*target);
            });
            lod->_ctx.log().infor("PCO: LOD attached nodes=%u",uint32(lod->_nodes.size()));
        }
    };
}
