#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <sstream>
#include <map>

#include "deploy.h"

using namespace  std;

#define maxn 1200
#define INF 100000000
#define maxpath 2000
#define maxserver 500
#define bigcase 800
#define smallcase 200
#define noRecv 10000//均加反向边

#ifdef _WIN32
#define ENTER "\n"
#endif

#ifdef __linux__
#define ENTER "\r\n"
#endif

struct Edge
{
    int from, to, cap, flow, cost;//入点、出点、容量、实际流量、流量单价
    Edge()=default;
    Edge(int u, int v, int c, int f,int w):
        from(u), to(v), cap(c), flow(f),cost(w){}
    bool operator ==(const Edge &e){
        return to=e.to;
    }
};

class EdgeFinder{
public:
    EdgeFinder(int fromNode,int toNode):m_from(fromNode),m_to(toNode){}
    bool operator()(Edge e){
        return e.to==m_to&&e.from==m_from;
    }
private:
    int m_from;
    int m_to;
};

struct Consumer{
    int no;//消费节点编号
    int netNode;//所连网络结点编号
    int flowNeed;//流量需求
};

struct Server{
    int id;//档次id
    int cost;//服务器成本
    int outCap;//输出容量
};

class Graph{
public:
    int nodeNum,edgeNum,consumerNum;//结点数
    vector<Edge> G[maxn];//网络结点邻接表
    vector<Consumer> consumers;//消费节点数组
    map<int,int> netToConsumer;//网络结点所连的消费结点
    vector<int>  netCost;//网络结点的部署成本
    vector<Server> serverInfo;//服务器档次信息
    //求最短路
    Edge p[maxn]; // 当前节点单源最短路中的上一条边
    int d[maxn]; // 单源最短路径长

    void init(int n,int m,int k){
        nodeNum=n;
        edgeNum=m;
        consumerNum=k;
        for(int i=0;i<nodeNum;i++){
            G[i].clear();
        }
        consumers.clear();

    }

    //获取图
    void createGraph(char * topo[MAX_EDGE_NUM]){
        string line=topo[0];
        stringstream ss(line);
        int nodeNum;//结点数
        int edgeNum;//边数
        int consumerNum;//消费结点数
        ss>>nodeNum>>edgeNum>>consumerNum;
        init(nodeNum,edgeNum,consumerNum);
        int lineCnt=2;
        //int serverCnt=0;
        //读取服务器信息
        for(;;){
            string line=topo[lineCnt];
            lineCnt++;
            if(line==string(ENTER))break;
            stringstream ss(line);
            Server server;
            ss>>server.id>>server.outCap>>server.cost;
            //if(serverCnt<3){
                serverInfo.push_back(server);
           //     serverCnt++;
           //}

        }
        //读取结点部署成本
        netCost.resize(nodeNum);
        for(;;){
            string line=topo[lineCnt];
            lineCnt++;
            if(line==string(ENTER))break;
            stringstream ss(line);
            int netNo,cost;
            ss>>netNo>>cost;
            netCost[netNo]=cost;
        }
        //读取链路信息
        for(;;){
            Edge e;
            string line=topo[lineCnt];
            lineCnt++;
            if(line==string(ENTER))break;
            stringstream ss(line);
            ss>>e.from>>e.to>>e.cap>>e.cost;
            e.flow=0;
            Edge rev_e=e;
            rev_e.from=e.to;
            rev_e.to=e.from;
            G[e.from].push_back(e);
            G[e.to].push_back(rev_e);
        }
        //读取消费结点信息
        for(int i=lineCnt;i<lineCnt+consumerNum;i++){
            Consumer consumer;
            string line=topo[i];
            stringstream ss(line);
            ss>>consumer.no>>consumer.netNode>>consumer.flowNeed;
            netToConsumer[consumer.netNode]=consumer.no;//建立网结点和消费结点映射
            consumers.push_back(consumer);
        }
    }


};

#endif // GRAPH_H
