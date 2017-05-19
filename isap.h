#ifndef ISAP_H
#define ISAP_H

#include <bits/stdc++.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stack>
#include <map>
#include <unistd.h>
#include <string.h>

using namespace std;

const int N = 1300;
const int inf = 0x3f3f3f3f;

struct Edge_ISAP
{
    int StarPoint; //边的起点
    int EndPoint;  //边的终点
    int Width;     //边的带宽
    int Cost;     //单位费用
    int flow;     //记录边上用的流量
};

class ISAP
{
    public:
    vector<int> G[N];
    vector<Edge_ISAP> Edges;
    bool Vist[N];
    int Dist[N];
    int num[N],cur[N],Pre[N];

    public:
    ISAP()
    {

    }

    ~ISAP()
    {

    }

    void addEdge(int StarPoint, int EndPoint, int Width, int Cost)
    {
        Edge_ISAP temp1 = { StarPoint,EndPoint,Width,Cost,0 };
        Edge_ISAP temp2 = { EndPoint,StarPoint,0,-Cost,0 };   //允许反向增广
    	Edges.push_back(temp1);
    	Edges.push_back(temp2);
    	int len = Edges.size();
    	G[StarPoint].push_back(len - 2);
    	G[EndPoint].push_back(len - 1);
    }

    //获取ISAP(转存Graph到ZKW的edge)
    void createISAP(const Graph &graph,const vector<int> servers)
    {
        for (int i = 0; i<graph.nodeNum; i++)
        {
            for(size_t j=0;j<graph.G[i].size();j++){
                addEdge(graph.G[i][j].from,graph.G[i][j].to,
                        graph.G[i][j].cap,graph.G[i][j].cost);

            }
        }
        int maxCap=graph.serverInfo[graph.serverInfo.size()-1].outCap;
        vector<int> consumerNetNodes,flowNeed;
        int flowNeedAll=0;
        for(int i=0;i<graph.consumerNum;i++){
            consumerNetNodes.push_back(graph.consumers[i].netNode);
            flowNeed.push_back(graph.consumers[i].flowNeed);
            flowNeedAll+=graph.consumers[i].flowNeed;
        }
        int superConsumerNetNode=graph.nodeNum+1;//超级汇
        for(size_t i=0;i<consumerNetNodes.size();i++){
            addEdge(consumerNetNodes[i],superConsumerNetNode,flowNeed[i],0);
        }
        int superServer=graph.nodeNum;//超级源
        for(size_t i=0;i<servers.size();i++){
            addEdge(superServer,servers[i],maxCap,0);
        }
    }

    bool iBFS(int StartNode , int EndNode)
    {
        memset(Vist, 0, sizeof(Vist));
        memset(Dist, 0, sizeof(Dist));
        queue<int> Q;
        Q.push(EndNode);
        Vist[EndNode] = 1;
        Dist[EndNode] = 0;
        while (!Q.empty()) {
            int TopNode = Q.front();
              Q.pop();
           for (int i = 0; i < G[TopNode].size(); i++){
                 Edge_ISAP & e = Edges[G[TopNode][i]];
                if (e.Width > e.flow && ! Vist[e.EndPoint] ){
                        Vist[e.EndPoint] = true;
                        Dist[e.EndPoint] = Dist[TopNode] + 1;
                        Q.push(e.EndPoint);
                }
            }
        }
        return Vist[StartNode];
    }

    pair<int,int> multiMinCostFlow(int StartNode,int EndNode,int NodeNum)
    {
      int flow=0,df= inf;
      int UnitCost=0,TotalCost=0;
      bool flag = false;
      pair<int,int> TempCostFlow;
      iBFS(StartNode,EndNode);
      memset(cur,0,sizeof(cur));
      memset(num,0,sizeof(num));
      for(int i = 0; i <NodeNum+2;i++){  //汇点和源点
         num[Dist[i]]++;}
      int u = StartNode;
      while(Dist[StartNode]<NodeNum+2){
           if(u==EndNode){
               flow += df;
               while(u!=StartNode){
               Edges[Pre[u]].flow += df;
               Edges[Pre[u]^1].flow -= df;
               UnitCost+=Edges[Pre[u]].Cost;
               u = Edges[Pre[u]].StarPoint;
             }
            TotalCost += UnitCost*df;
            UnitCost =0;
            u = StartNode;
            df= inf;
          }
        flag = false;
       for(int i = cur[u]; i<G[u].size();i++){
             Edge_ISAP &e = Edges[G[u][i]];
             if(e.Width>e.flow&&Dist[u]==Dist[e.EndPoint]+1){
                flag = true;
                Pre[e.EndPoint] = G[u][i];
                df = min(df,e.Width-e.flow);
                cur[u] = i;   //记录遍历出度 的第几个出度
                u = e.EndPoint;
                break;
            }
        }
      if(!flag){
            int m = NodeNum+1;
               for(int j=0;j<G[u].size();j++)
               if(Edges[G[u][j]].Width > Edges[G[u][j]].flow){
                m = min(m,Dist[Edges[G[u][j]].EndPoint]);
               }
                if(--num[Dist[u]] == 0) break;
                num[Dist[u]= m+1]++;
                cur[u] = 0;
                if(u != StartNode)
                    u = Edges[Pre[u]].StarPoint;
            }
       }
       TempCostFlow.first = TotalCost;
       TempCostFlow.second= flow;
       //cout << TotalCost << " " << flow << endl;
       return TempCostFlow;
    }
};

#endif
