
#pragma     once
#include    "../../fileFormat/svg/FESVGTypes.hpp"
#include    <algorithm>
#include    <cmath>
#include    <cfloat>
#include    <vector>

namespace   FE
{
    /// <summary>
    /// 多边形三角化(带洞),算法为 earcut(mapbox 版本)的 C++ 移植:
    /// 1. 外轮廓与各洞分别构建双向链接环,统一外环逆时针、洞顺时针;
    /// 2. 洞按最左点排序,用向左水平射线找到外环上的桥接顶点,把洞"缝"进外环;
    /// 3. 反复过滤共线点并切耳,必要时通过局部处理与递归切分收敛。
    /// 输出索引指向 outPoints(外轮廓点 + 各洞点按传入顺序拼接)。
    /// </summary>
    class   FEEarcut
    {
    public:
        /// <summary>
        /// 三角化多边形
        /// </summary>
        /// <param name="outer">外轮廓(首尾不重复)</param>
        /// <param name="holes">洞集合(每个首尾不重复)</param>
        /// <param name="outPoints">可选,输出索引对应的点集</param>
        /// <returns>三角形索引,每 3 个一组</returns>
        static  uints   triangulate(const float2s& outer,const std::vector<float2s>& holes,
                                    float2s* outPoints = nullptr)
        {
            Impl    impl;
            uints   result  =   impl.run(outer,holes,outPoints);
            return  result;
        }

    private:
        class   Impl
        {
        public:
            struct  Node
            {
                int     i       =   -1;     /// 点索引
                int     prev    =   -1;
                int     next    =   -1;
                float   x       =   0.0f;
                float   y       =   0.0f;
                bool    steiner =   false;
            };
        public:
            uints   run(const float2s& outer,const std::vector<float2s>& holes,float2s* outPoints)
            {
                if (outer.size() < 3)
                    return  {};

                _points =   outer;
                int     outerNode   =   linkedList(0,(int)outer.size(),true);
                if (outerNode < 0)
                    return  {};

                std::vector<int>    holeStarts;
                for (const auto& hole : holes)
                {
                    if (hole.size() < 3)
                        continue;

                    const   int start   =   (int)_points.size();
                    _points.insert(_points.end(),hole.begin(),hole.end());
                    int     list    =   linkedList(start,(int)_points.size(),false);
                    if (list < 0)
                        continue;
                    if (_n[list].next == list)
                        _n[list].steiner    =   true;
                    holeStarts.push_back(getLeftmost(list));
                }

                /// 洞按最左点排序,保证射线桥接的单调性
                std::sort(holeStarts.begin(),holeStarts.end(),[&](int a,int b)
                {
                    return  _n[a].x < _n[b].x;
                });
                for (int hole : holeStarts)
                {
                    outerNode   =   eliminateHole(hole,outerNode);
                    outerNode   =   filterPoints(outerNode,_n[outerNode].next);
                }

                earcutLinked(outerNode,0);

                if (outPoints != nullptr)
                    *outPoints  =   std::move(_points);
                return  _tris;
            }
        private:
            std::vector<Node>   _n;
            float2s             _points;
            uints               _tris;
        private:
            int     newNode(int i,float x,float y)
            {
                Node    node;
                node.i  =   i;
                node.x  =   x;
                node.y  =   y;
                _n.push_back(node);
                return  (int)_n.size() - 1;
            }
            int     insertNode(int i,const float2& p,int last)
            {
                const   int idx =   newNode(i,p.x,p.y);
                if (last < 0)
                {
                    _n[idx].prev    =   idx;
                    _n[idx].next    =   idx;
                }
                else
                {
                    _n[idx].next            =   _n[last].next;
                    _n[idx].prev            =   last;
                    _n[_n[last].next].prev  =   idx;
                    _n[last].next           =   idx;
                }
                return  idx;
            }
            void    removeNode(int p)
            {
                _n[_n[p].next].prev =   _n[p].prev;
                _n[_n[p].prev].next =   _n[p].next;
            }
            static  float   signedArea(const float2s& pts,int start,int end)
            {
                float   sum =   0.0f;
                for (int i = start, j = end - 1; i < end; j = i++)
                    sum +=  (pts[j].x - pts[i].x) * (pts[i].y + pts[j].y);
                return  sum;
            }
            int     linkedList(int start,int end,bool clockwise)
            {
                int     last    =   -1;
                if (clockwise == (signedArea(_points,start,end) > 0.0f))
                {
                    for (int i = start; i < end; ++i)
                        last    =   insertNode(i,_points[i],last);
                }
                else
                {
                    for (int i = end - 1; i >= start; --i)
                        last    =   insertNode(i,_points[i],last);
                }

                if (last >= 0 && equals(last,_n[last].next))
                {
                    removeNode(last);
                    last    =   _n[last].next;
                }
                return  last;
            }
            float   area(int p,int q,int r) const
            {
                return  (_n[q].y - _n[p].y) * (_n[r].x - _n[q].x) -
                        (_n[q].x - _n[p].x) * (_n[r].y - _n[q].y);
            }
            bool    equals(int a,int b) const
            {
                return  _n[a].x == _n[b].x && _n[a].y == _n[b].y;
            }
            bool    onSegment(int p,int q,int r) const
            {
                return  _n[q].x <= (std::max)(_n[p].x,_n[r].x) &&
                        _n[q].x >= (std::min)(_n[p].x,_n[r].x) &&
                        _n[q].y <= (std::max)(_n[p].y,_n[r].y) &&
                        _n[q].y >= (std::min)(_n[p].y,_n[r].y);
            }
            static  int     sign(float v)
            {
                return  v > 0.0f ? 1 : (v < 0.0f ? -1 : 0);
            }
            bool    intersects(int p1,int q1,int p2,int q2) const
            {
                const   int o1  =   sign(area(p1,q1,p2));
                const   int o2  =   sign(area(p1,q1,q2));
                const   int o3  =   sign(area(p2,q2,p1));
                const   int o4  =   sign(area(p2,q2,q1));

                if (o1 != o2 && o3 != o4)
                    return  true;

                if (o1 == 0 && onSegment(p1,p2,q1))
                    return  true;
                if (o2 == 0 && onSegment(p1,q2,q1))
                    return  true;
                if (o3 == 0 && onSegment(p2,p1,q2))
                    return  true;
                if (o4 == 0 && onSegment(p2,q1,q2))
                    return  true;
                return  false;
            }
            bool    intersectsPolygon(int a,int b) const
            {
                int     p   =   a;
                do
                {
                    if (_n[p].i != _n[a].i && _n[_n[p].next].i != _n[a].i &&
                        _n[p].i != _n[b].i && _n[_n[p].next].i != _n[b].i &&
                        intersects(p,_n[p].next,a,b))
                        return  true;
                    p   =   _n[p].next;
                } while (p != a);
                return  false;
            }
            bool    locallyInside(int a,int b) const
            {
                return  area(_n[a].prev,a,_n[a].next) < 0.0f ?
                        (area(a,b,_n[a].next) >= 0.0f && area(a,_n[a].prev,b) >= 0.0f) :
                        (area(a,b,_n[a].prev) < 0.0f || area(a,_n[a].next,b) < 0.0f);
            }
            bool    middleInside(int a,int b) const
            {
                int     p       =   a;
                bool    inside  =   false;
                const   float   px  =   (_n[a].x + _n[b].x) * 0.5f;
                const   float   py  =   (_n[a].y + _n[b].y) * 0.5f;
                do
                {
                    if (((_n[p].y > py) != (_n[_n[p].next].y > py)) &&
                        _n[_n[p].next].y != _n[p].y &&
                        (px < (_n[_n[p].next].x - _n[p].x) * (py - _n[p].y) /
                              (_n[_n[p].next].y - _n[p].y) + _n[p].x))
                        inside  =   !inside;
                    p   =   _n[p].next;
                } while (p != a);
                return  inside;
            }
            bool    isValidDiagonal(int a,int b) const
            {
                return  _n[_n[a].next].i != _n[b].i && _n[_n[a].prev].i != _n[b].i &&
                        !intersectsPolygon(a,b) &&
                        ((locallyInside(a,b) && locallyInside(b,a) && middleInside(a,b) &&
                          (area(_n[a].prev,a,_n[b].prev) != 0.0f || area(a,_n[b].prev,b) != 0.0f)) ||
                         (equals(a,b) && area(_n[a].prev,a,_n[a].next) > 0.0f &&
                          area(_n[b].prev,b,_n[b].next) > 0.0f));
            }
            int     splitPolygon(int a,int b)
            {
                const   int a2  =   newNode(_n[a].i,_n[a].x,_n[a].y);
                const   int b2  =   newNode(_n[b].i,_n[b].x,_n[b].y);
                const   int an  =   _n[a].next;
                const   int bp  =   _n[b].prev;

                _n[a].next  =   b;
                _n[b].prev  =   a;

                _n[a2].next =   an;
                _n[an].prev =   a2;

                _n[b2].next =   a2;
                _n[a2].prev =   b2;

                _n[bp].next =   b2;
                _n[b2].prev =   bp;
                return  b2;
            }
            bool    pointInTriangle(float ax,float ay,float bx,float by,float cx,float cy,
                                    float px,float py) const
            {
                return  (cx - px) * (ay - py) - (ax - px) * (cy - py) >= 0.0f &&
                        (ax - px) * (by - py) - (bx - px) * (ay - py) >= 0.0f &&
                        (bx - px) * (cy - py) - (cx - px) * (by - py) >= 0.0f;
            }
            bool    isEar(int ear) const
            {
                const   int a   =   _n[ear].prev;
                const   int b   =   ear;
                const   int c   =   _n[ear].next;

                /// 凹点不能作为耳朵(earcut 使用相反的面积符号约定)
                if (area(a,b,c) >= 0.0f)
                    return  false;

                int     p   =   _n[c].next;
                while (p != a)
                {
                    if (pointInTriangle(_n[a].x,_n[a].y,_n[b].x,_n[b].y,_n[c].x,_n[c].y,
                                        _n[p].x,_n[p].y) &&
                        area(_n[p].prev,p,_n[p].next) >= 0.0f)
                        return  false;
                    p   =   _n[p].next;
                }
                return  true;
            }
            int     filterPoints(int start,int end)
            {
                if (start < 0)
                    return  start;
                if (end < 0)
                    end =   start;

                int     p       =   start;
                bool    again   =   true;
                do
                {
                    again   =   false;
                    if (!_n[p].steiner &&
                        (equals(p,_n[p].next) || area(_n[p].prev,p,_n[p].next) == 0.0f))
                    {
                        removeNode(p);
                        p   =   end =   _n[p].prev;
                        if (p == _n[p].next)
                            break;
                        again   =   true;
                    }
                    else
                    {
                        p   =   _n[p].next;
                    }
                } while (again || p != end);
                return  end;
            }
            int     getLeftmost(int start) const
            {
                int     p       =   start;
                int     left    =   start;
                do
                {
                    if (_n[p].x < _n[left].x || (_n[p].x == _n[left].x && _n[p].y < _n[left].y))
                        left    =   p;
                    p   =   _n[p].next;
                } while (p != start);
                return  left;
            }
            int     findHoleBridge(int hole,int outerNode)
            {
                const   float   hx  =   _n[hole].x;
                const   float   hy  =   _n[hole].y;

                int     p       =   outerNode;
                float   qx      =   -FLT_MAX;
                int     m       =   -1;
                do
                {
                    if (hy <= _n[p].y && hy >= _n[_n[p].next].y && _n[_n[p].next].y != _n[p].y)
                    {
                        const   float   x   =   _n[p].x + (hy - _n[p].y) *
                                                (_n[_n[p].next].x - _n[p].x) /
                                                (_n[_n[p].next].y - _n[p].y);
                        if (x <= hx && x > qx)
                        {
                            qx  =   x;
                            m   =   _n[p].x < _n[_n[p].next].x ? p : _n[p].next;
                            if (x == hx)
                                return  m;
                        }
                    }
                    p   =   _n[p].next;
                } while (p != outerNode);

                if (m < 0)
                    return  -1;
                if (hx == qx)
                    return  m;

                /// 在候选点附近寻找更合适的反射点(射线被遮挡时)
                const   int     stop    =   m;
                const   float   mx      =   _n[m].x;
                const   float   my      =   _n[m].y;
                float           tanMin  =   FLT_MAX;

                p   =   m;
                do
                {
                    if (hx >= _n[p].x && _n[p].x >= mx && hx != _n[p].x &&
                        pointInTriangle(hy < my ? hx : qx,hy,mx,my,hy < my ? qx : hx,hy,
                                        _n[p].x,_n[p].y))
                    {
                        const   float   tan =   std::fabs(hy - _n[p].y) / (hx - _n[p].x);
                        if (locallyInside(p,hole) &&
                            (tan < tanMin || (tan == tanMin && _n[p].x > _n[m].x)))
                        {
                            m       =   p;
                            tanMin  =   tan;
                        }
                    }
                    p   =   _n[p].next;
                } while (p != stop);
                return  m;
            }
            int     eliminateHole(int hole,int outerNode)
            {
                const   int bridge  =   findHoleBridge(hole,outerNode);
                if (bridge < 0)
                    return  outerNode;

                const   int bridgeReverse   =   splitPolygon(bridge,hole);
                filterPoints(bridgeReverse,_n[bridgeReverse].next);
                return  filterPoints(bridge,_n[bridge].next);
            }
            int     cureLocalIntersections(int start)
            {
                int     p   =   start;
                do
                {
                    const   int a   =   _n[p].prev;
                    const   int b   =   _n[_n[p].next].next;
                    if (intersects(a,p,_n[p].next,b) && locallyInside(a,b) && locallyInside(b,a))
                    {
                        _tris.push_back(_n[a].i);
                        _tris.push_back(_n[p].i);
                        _tris.push_back(_n[b].i);

                        removeNode(p);
                        removeNode(_n[p].next);
                        p   =   start   =   b;
                    }
                    p   =   _n[p].next;
                } while (p != start);
                return  filterPoints(p,-1);
            }
            void    splitEarcut(int start)
            {
                int     a   =   getLeftmost(start);
                do
                {
                    int     b   =   _n[_n[a].next].next;
                    while (b != _n[a].prev)
                    {
                        if (isValidDiagonal(a,b))
                        {
                            int     c   =   splitPolygon(a,b);
                            a   =   filterPoints(a,_n[a].next);
                            c   =   filterPoints(c,_n[c].next);
                            earcutLinked(a,0);
                            earcutLinked(c,0);
                            return;
                        }
                        b   =   _n[b].next;
                    }
                    a   =   _n[a].next;
                } while (a != start);
            }
            void    earcutLinked(int ear,int pass)
            {
                if (ear < 0)
                    return;

                int     stop    =   ear;
                while (_n[ear].prev != _n[ear].next)
                {
                    const   int prev    =   _n[ear].prev;
                    const   int next    =   _n[ear].next;
                    if (isEar(ear))
                    {
                        _tris.push_back(_n[prev].i);
                        _tris.push_back(_n[ear].i);
                        _tris.push_back(_n[next].i);
                        removeNode(ear);
                        ear     =   next;
                        stop    =   next;
                        continue;
                    }

                    ear =   next;
                    if (ear == stop)
                    {
                        if (pass == 0)
                        {
                            earcutLinked(filterPoints(ear,-1),1);
                        }
                        else if (pass == 1)
                        {
                            ear =   cureLocalIntersections(ear);
                            earcutLinked(ear,2);
                        }
                        else if (pass == 2)
                        {
                            splitEarcut(ear);
                        }
                        break;
                    }
                }
            }
        };
    };
}
