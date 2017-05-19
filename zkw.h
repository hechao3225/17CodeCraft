#ifndef ZKW_H
#define ZKW_H

#include <cstdio>
#include <cstring>
#include <iostream>
#include <stack>

#include "graph.h"
using namespace std;

const int MAXN = 1300;
const int MAXM = 20000;
struct Edge_ZKW
{
    int to,next,cap,flow,cost;
    Edge_ZKW(int toNode = 0,int nextRecord = 0,int allCap = 0,int flowRecord = 0,int unitCost = 0):
        to(toNode),next(nextRecord),cap(allCap),flow(flowRecord),cost(unitCost){}
};

struct ZKW_MinCost//转换为最小费用流
 {
public:
    int nodeNum,edgeNum,consumerNum;
    int headData[MAXN],cnt;
    int curNodeData[MAXN];
    int distData[MAXN];
    bool visit[MAXN];
    int startNode,endNode,N;//源点、汇点和点的总个数（编号是0~N-1）,不需要额外赋值，调用会直接赋值
    int minCostData, maxFlowData;
    vector<Edge_ZKW> zkwEdges;
    vector<int> preMinPath[maxpath];
    int fixFlag=0;
    pair<int,int> fixNodes;

    stack<int> stk;//用于存储路径(*)

    void init(int n,int m,int cn)
    {
        nodeNum=n;
        edgeNum=m;
        consumerNum=cn;
        cnt = 0;
        memset(headData,-1,sizeof(headData));
    }

    void addEdge(int u,int v,int cap,int cost)
    {
        zkwEdges.push_back(Edge_ZKW(v,headData[u],cap,0,cost));
        headData[u] = cnt++;
        zkwEdges.push_back(Edge_ZKW(u,headData[v],0,0,-cost));
        headData[v] = cnt++;
    }

    void addEdge_NoRecv(int u,int v,int cap,int cost)
    {
        zkwEdges.push_back(Edge_ZKW(v,headData[u],cap,0,cost));
        headData[u] = cnt++;
    }

    //获取ZKW(转存Graph到ZKW的edge)
    void createZKW(const Graph &graph,bool isRecv){
        init(graph.nodeNum,graph.edgeNum,graph.consumerNum);
        for (int i = 0; i<graph.nodeNum; i++)
        {
            for(size_t j=0;j<graph.G[i].size();j++){
                if(isRecv)
                addEdge(graph.G[i][j].from,graph.G[i][j].to,
                        graph.G[i][j].cap,graph.G[i][j].cost);
                else
                addEdge_NoRecv(graph.G[i][j].from,graph.G[i][j].to,
                        graph.G[i][j].cap,graph.G[i][j].cost);

            }
        }
    }

    int myaug(int u,int flow)//修改后的增广过程，用于输出路径(*)
    {
        if(u == endNode)
        {
            stk.push(u);
            return flow;
        }

        visit[u] = true;
        for(int i = curNodeData[u];i != -1;i = zkwEdges[i].next)
        {
            int v = zkwEdges[i].to;
            if(!visit[v] && zkwEdges[i].flow > 0)
            {
                int tmp = myaug(v,min(flow,zkwEdges[i].flow));//残量网络中取最小值作为增广流量

                curNodeData[u] = i;
                if(tmp)
                {
                    stk.push(u);
                    zkwEdges[i].flow -= tmp;
                    //zkwEdges[i^1].flow += tmp;
                    return tmp;
                }
            }

        }
        return 0;
    }

    int augmentProcess(int u,int flow,bool isSavePath,bool isRecv)
    {
        if(u == endNode)return flow;
        visit[u] = true;
        for(int i = curNodeData[u];i != -1;i = zkwEdges[i].next)
        {
            int v = zkwEdges[i].to;
            if(zkwEdges[i].cap > zkwEdges[i].flow && !visit[v] && distData[u] == distData[v] + zkwEdges[i].cost)
            {
                int tmp = augmentProcess(v,min(flow,zkwEdges[i].cap-zkwEdges[i].flow),isSavePath,isRecv);//残量网络中取最小值作为增广流量
                zkwEdges[i].flow += tmp;//正向边增加流量
                if(isRecv)zkwEdges[i^1].flow -= tmp;//反向边减小流量
                curNodeData[u] = i;

                if(tmp){
/*
                    if(edges[i].cap==0){
                        cout<<"u,v:"<<u<<","<<v<<endl;
                    }
*/
                    if(isSavePath)stk.push(u);
                    return tmp;
                }
            }

        }
        return 0;
    }

    bool modifyLaybel()
    {
        int d = INF;
        for(int u = 0;u < N;u++){
            if(visit[u]){
                for(int i = headData[u];i != -1;i = zkwEdges[i].next)
                {
                    int v = zkwEdges[i].to;

                    if(zkwEdges[i].cap>zkwEdges[i].flow && !visit[v]){
                        d = min(d,distData[v]+zkwEdges[i].cost-distData[u]);
                    }
                }
            }
        }
        if(d == INF)return false;
        for(int i = 0;i < N;i++)
            if(visit[i])
            {
                visit[i] = false;
                distData[i] += d;

            }
        return true;
    }      

    pair<int,int> minCostMaxFlow(int start,int end,int n){
        startNode = start, endNode = end, N = n;
        minCostData = maxFlowData = 0;
        for(int i = 0;i < n;i++)distData[i] = 0;
        while(1)
        {
            for(int i = 0;i < n;i++)curNodeData[i] = headData[i];
            while(1)
            {
                for(int i = 0;i < n;i++)visit[i] = false;
                int tmp = augmentProcess(startNode,INF,true,true);
                if(tmp == 0)break;
                maxFlowData += tmp;
                minCostData += tmp*distData[startNode];
            }
            if(!modifyLaybel())break;
        }
        return make_pair(minCostData,maxFlowData);
    }

    //单源单汇最小费用流(返回费用和流量)
    pair<int,int> minCostFlow(int start,int end,int flow,int n,int addNode){
        //可通过增加汇点转换为最小费用流
        addEdge(end,addNode,flow,0);
        return minCostMaxFlow(start,addNode,n);
    }

    //多源多汇最小费用流(v1:存路径)
    pair<int,int> multiMinCostFlow(const vector<int> &servers,const vector<int> &consumerNetNodes,
                                   const vector<int> &flowNeed,int maxCap,int n,
                                   vector<int > minCostPath[maxpath],int &m,bool isRecv){

        int superConsumerNetNode=nodeNum+1;//超级汇
        for(size_t i=0;i<consumerNetNodes.size();i++){
            addEdge(consumerNetNodes[i],superConsumerNetNode,flowNeed[i],0);
        }
        int superServer=nodeNum;//超级源
        for(size_t i=0;i<servers.size();i++){
            addEdge(superServer,servers[i],maxCap,0);
        }

        //return minCostMaxFlow(superServer,superConsumerNetNode,n+2,minCostPath,m);
        startNode = superServer, endNode = superConsumerNetNode, N = n+2;
        minCostData = maxFlowData = 0;
        int pathCnt=0;
        for(int i = 0;i < n+2;i++)distData[i] = 0;
        for(;;)
        {
            for(int i = 0;i < n+2;i++)curNodeData[i] = headData[i];
            for(;;)
            {
                for(int i = 0;i < n+2;i++)visit[i] = false;
                int tmp = augmentProcess(startNode,INF,true,isRecv);

                if(tmp){
                    while(!stk.empty()){//无反向边时的路径输出(*)
                        int node=stk.top();
                        stk.pop();
                        if(node==superServer)continue;//无反向边：剔除超级源(*)
                        if(!isRecv)minCostPath[pathCnt].push_back(node);//无反向边：存路径(*)
                        //cout<<node<<' ';
                    }
                    if(!isRecv)minCostPath[pathCnt].push_back(tmp);//无反向边：存流量(*)
                    //cout<<"flow:"<<tmp<<endl;
                    if(!isRecv)pathCnt++;

                }

                if(tmp == 0)break;
                maxFlowData += tmp;
                minCostData += tmp*distData[startNode];
            }
            if(!modifyLaybel())break;
        }
        if(isRecv){//加反向边的情况：根据残余网络的流量信息存储路径(*)
            for(int i = 0;i < n+2 ;i++)curNodeData[i] = headData[i];
            for(;;)
            {
                for(int i = 0;i < n+2 ;i++)visit[i] = false;
                int tmp = myaug(startNode,INF);
                if(tmp){
                    vector<int> v;
                    while(!stk.empty()){//有反向边时的路径输出(*)
                        int node=stk.top();
                        stk.pop();
                        if(node==superServer)continue;//有反向边：剔除超级源(*)
                        if(node==superConsumerNetNode)continue;//有反向边：剔除超级汇(*)
                        v.push_back(node);//有反向边：存路径(*)
                    }
                    v.push_back(tmp);//有反向边：存流量(*)
                    minCostPath[pathCnt]=v;
                    pathCnt++;
                }
                if(tmp == 0)break;

            }
        }
        m=pathCnt;
        return make_pair(minCostData,maxFlowData);
    }

    //多源多汇最小费用流(v2:不存路径)
    pair<int,int> multiMinCostFlow(const vector<int> &servers,const vector<int> &consumerNetNodes,
                                   const vector<int> &flowNeed,int maxCap,int n,bool isRecv){

        int superConsumerNetNode=nodeNum+1;//超级汇
        for(size_t i=0;i<consumerNetNodes.size();i++){
            addEdge(consumerNetNodes[i],superConsumerNetNode,flowNeed[i],0);
        }
        int superServer=nodeNum;//超级源
        for(size_t i=0;i<servers.size();i++){
            addEdge(superServer,servers[i],maxCap,0);
        }

        //return minCostMaxFlow(superServer,superConsumerNetNode,n+2,minCostPath,m);
        startNode = superServer, endNode = superConsumerNetNode, N = n+2;
        minCostData = maxFlowData = 0;
        for(int i = 0;i < n+2;i++)distData[i] = 0;
        for(;;)
        {
            for(int i = 0;i < n+2;i++)curNodeData[i] = headData[i];
            for(;;)
            {
                for(int i = 0;i < n+2;i++)visit[i] = false;
                int tmp = augmentProcess(startNode,INF,false,isRecv);
                if(tmp == 0)break;
                maxFlowData += tmp;
                minCostData += tmp*distData[startNode];
            }
            if(!modifyLaybel())break;
        }

        return make_pair(minCostData,maxFlowData);
    }
};

#endif // ZKW_H
