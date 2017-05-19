#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>

#include "solution.h"
#include "zkw.h"
#include "time.h"

using namespace std;

bool operator<(const Result &res1,const Result &res2){
    return res1.minCost<res2.minCost;
}

bool cmpByBand(const pair<int,int> &p1,const pair<int,int> &p2){//输出能力高的排前
    return p1.second>p2.second;
}

void Solution::init(const Graph &graph,const Result &firstResult,double leftTime)
{
    vector<int> consumerNetNodes,flowNeed;
    int flowNeedAll=0;
    for(int i=0;i<graph.consumerNum;i++){
        consumerNetNodes.push_back(graph.consumers[i].netNode);
        flowNeed.push_back(graph.consumers[i].flowNeed);
        flowNeedAll+=graph.consumers[i].flowNeed;
    }
    bestRes=firstResult;
    serverLocations.push(firstResult);
    middleMaxTime=leftTime;
    bigMaxTime=leftTime;
    cout<<"left Time:"<<bigMaxTime<<endl;
}

void Solution::init(const Graph &graph)
{
    vector<int> consumerNetNodes,flowNeed;
    int flowNeedAll=0;
    for(int i=0;i<graph.consumerNum;i++){
        consumerNetNodes.push_back(graph.consumers[i].netNode);
        flowNeed.push_back(graph.consumers[i].flowNeed);
        flowNeedAll+=graph.consumers[i].flowNeed;
    }
    vector<bool> firstNetNodes(graph.nodeNum,false);
    for(int i=0;i<graph.consumerNum;i++){
        firstNetNodes[graph.consumers[i].netNode]=true;
    }
    Result first;
    first.netNodes=firstNetNodes;
    int minCost=0;
    for(int i=0;i<graph.consumerNum;i++){
        int curServerNode=graph.consumers[i].netNode;
        int serverID=findServerType(graph,graph.consumers[i].flowNeed);
        minCost+=graph.serverInfo[serverID].cost;
        minCost+=graph.netCost[curServerNode];
    }
    first.minCost=minCost;
    bestRes=first;
    serverLocations.push(first);
}

//根据流量消耗查询所属服务器档次，返回档次ID
int Solution::findServerType(const Graph &graph,int flow)
{
    for(int i=0;i<graph.serverInfo.size();i++){
        if(flow<=graph.serverInfo[i].outCap)
            return i;
    }
    return -1;
}

//计算解的费用并写入到该解
bool Solution::calMinCost(const Graph &graph,const vector<int> &consumerNetNodes,vector<int> &flowNeed,
                         int flowNeedAll,Result &result)
{

    ZKW_MinCost zkw;
    if(graph.nodeNum>=noRecv)
        zkw.createZKW(graph,false);
    else
        zkw.createZKW(graph,true);

    pair<int,int> res;
    vector<int> servers;
    for(int i=0;i<result.netNodes.size();i++){
        if(result.netNodes[i])servers.push_back(i);
    }
    int maxCap=graph.serverInfo[graph.serverInfo.size()-1].outCap;
    if(graph.nodeNum>=noRecv)
        res=zkw.multiMinCostFlow(servers,consumerNetNodes,flowNeed,maxCap,graph.nodeNum,false);
    else
        res=zkw.multiMinCostFlow(servers,consumerNetNodes,flowNeed,maxCap,graph.nodeNum,true);
    if(res.second==flowNeedAll){
        int minCost=res.first;
        int serverCost=0,layoutCost=0;
        int edgeSize=zkw.zkwEdges.size();
        for(int i=2,j=1;j<=servers.size();i+=2,j++){
            int curServerNode=servers[servers.size()-j];
            int curFlow=zkw.zkwEdges[edgeSize-i].flow;
            int serverID;
            serverID=findServerType(graph,curFlow);//根据流量消耗判断服务器档次
            if(serverID==-1)return false;//某个服务器输出流量超过最高档次的服务器输出能力
            serverCost+=graph.serverInfo[serverID].cost;//加上该档次服务器的成本
            layoutCost+=graph.netCost[curServerNode];//加上服务器所在结点的部署成本
        }
        minCost+=serverCost;
        minCost+=layoutCost;
        result.minCost=(double)minCost;//基本适应度为minCost
        return true;
    }

    return false;//非有效个体（不是可行解）
}

void Solution::exchangeServer(const Result &motherRes,Result &res,const Graph &graph)
{
    res=motherRes;
    int randServerNode,randOtherNode=-1;
    do{
        randServerNode=rand()%res.netNodes.size();
    }
    while(!res.netNodes[randServerNode]);
    int layerNum=rand()%5+1;
    vector<bool> mark(motherRes.netNodes.size(),false);
    vector<int> proNodes;
    vector<int> layer={randServerNode};
    while(layerNum-->0){
        vector<int> temp;
        for(auto node:layer){
            for(int i=0;i<graph.G[node].size();i++){
                int adjNode=graph.G[node][i].to;
                if(!mark[adjNode] && !res.netNodes[adjNode]){//未访问过且为非服务器结点
                    temp.push_back(adjNode);
                    mark[adjNode]=true;
                }
            }
        }
        for(auto node:temp){
            proNodes.push_back(node);
        }
        temp.swap(layer);
    }
    if(!proNodes.empty()){
        int randNum=rand()%proNodes.size();
        randOtherNode=proNodes[randNum];
    }
    res.netNodes[randServerNode]=false;
    if(randOtherNode!=-1){
        res.netNodes[randOtherNode]=true;
    }

}

void Solution::increaseServer(const Result &motherRes,Result &res)
{
    res=motherRes;
    int randOtherNode;
    do{
        randOtherNode=rand()%res.netNodes.size();
    }
    while(res.netNodes[randOtherNode]);
    res.netNodes[randOtherNode]=true;

}

void Solution::deleteServer(const Graph &graph,const Result &motherRes,Result &res)
{
    res=motherRes;
    int randServerNode;
    int deleteNum=1;
    int deleteUp=20;
    int deleteDown=10;
    if(graph.nodeNum>=bigcase)deleteNum=rand()%(deleteUp-deleteDown+1)+deleteDown;
    if(graph.nodeNum<bigcase&&graph.nodeNum>smallcase)deleteNum=2;
    for(int i=0;i<deleteNum;i++){
        do{
            randServerNode=rand()%res.netNodes.size();
        }
        while(!res.netNodes[randServerNode]);
        res.netNodes[randServerNode]=false;
    }
}

void Solution::solve(const Graph &graph)
{
    clock_t timeout_start=clock();
    vector<int> consumerNetNodes,flowNeed;
    int flowNeedAll=0;
    for(int i=0;i<graph.consumerNum;i++){
        consumerNetNodes.push_back(graph.consumers[i].netNode);
        flowNeed.push_back(graph.consumers[i].flowNeed);
        flowNeedAll+=graph.consumers[i].flowNeed;
    }

    int op[3]={0,1,2};//0：交换 1：增加 2：删除
    int opSelect=op[0];//默认交换
    double opRate[3]={0.0,0.3,0.7};//0.4 0.1 0.5
    int nicerDist=0;
    int resultNum=50;
    int zkwCnt=0;

    double d_time_before=0.0;
    if(graph.nodeNum>=bigcase)resultNum=50;
    if(graph.nodeNum<bigcase&&graph.nodeNum>smallcase)resultNum=300;
    if(graph.nodeNum<=smallcase)resultNum=500;

    if(graph.nodeNum>=bigcase){
        opRate[0]=0.3;
        opRate[1]=0.1;
        opRate[2]=0.6;
    }
    //迭代：
    for(int i=0;i<times;i++){
        clock_t d_finish=clock();
        double d_time = (double)(d_finish-timeout_start)/CLOCKS_PER_SEC*1000;

        if(d_time-d_time_before>1){//每1s增加1个子代个体
            //if(resultNum<100&&graph.nodeNum>=bigcase)resultNum++;
            //if(graph.nodeNum<bigcase&&graph.nodeNum>smallcase&&resultNum<500)resultNum++;
            //if(graph.nodeNum<=smallcase&&resultNum<1000)resultNum++;
            d_time_before=d_time;
        }
        Result motherRes,childRes;
        //选出待操作的可行解
        if(!serverLocations.empty()){
            motherRes=serverLocations.top();
            serverLocations.pop();
        }
        else{
            motherRes=bestRes;
        }
        //产生一群子代
        set<Result> childResults;
        for(int num=0;num<resultNum;num++){
            //轮盘赌选择一种操作
            int m=0;
            int randNum=rand()%1001;//[0,1000]
            for(int i=0;i<3;i++){
                m+=opRate[i]*1000;
                if(randNum<m){
                    opSelect=op[i];
                    break;
                }
            }
            //随机进行一种操作
            if(opSelect==0){
                exchangeServer(motherRes,childRes,graph);
            }
            else if(opSelect==1){
                increaseServer(motherRes,childRes);
            }
            else if(opSelect==2){
                deleteServer(graph,motherRes,childRes);
            }

            //更新操作后的解集
            if(calMinCost(graph,consumerNetNodes,flowNeed,flowNeedAll,childRes)){
                if(newResults.find(childRes)==newResults.end()){
                    newResults.insert(childRes);
                    childResults.insert(childRes);
                }

            }
            zkwCnt++;
        }
        for(auto e:childResults){
            if(e.minCost<bestRes.minCost-nicerDist){
                bestRes=e;
            }
            serverLocations.push(e);
        }
        clock_t timeout_finish=clock();
        double currentTime = (double)(timeout_finish-timeout_start)/CLOCKS_PER_SEC;
        if(graph.nodeNum<bigcase&&graph.nodeNum>smallcase&&currentTime>=middleMaxTime){
            cout<<i<<"time out:"<<currentTime<<endl;
            break;
        }
        if(graph.nodeNum>=bigcase&&currentTime>=bigMaxTime){
            cout<<i<<"time out:"<<currentTime<<endl;
            break;
        }
        if(graph.nodeNum<=smallcase&&currentTime>=smallMaxTime){
            cout<<i<<"time out:"<<currentTime<<endl;
            break;
        }
        cout<<i<<" "<<motherRes.minCost<<" find cost:"<<bestRes.minCost
            <<" zkw called:"<<zkwCnt<<"current time:"<<currentTime<<endl;
    }


}

void Solution::getResult(const Result &bestRes,const Graph &graph,
                      int &lastMinCost,vector<vector<int> > &lastMinPath,
                      map<int,int> &lastServers)
{
    vector<int> consumerNetNodes,flowNeed;
    int flowNeedAll=0;
    for(int i=0;i<graph.consumerNum;i++){
        consumerNetNodes.push_back(graph.consumers[i].netNode);
        flowNeed.push_back(graph.consumers[i].flowNeed);
        flowNeedAll+=graph.consumers[i].flowNeed;
    }

    ZKW_MinCost zkw;
    if(graph.nodeNum>=noRecv)
        zkw.createZKW(graph,false);
    else
        zkw.createZKW(graph,true);
    pair<int,int> res;
    vector<int> servers;
    //cout<<"last servers:"<<endl;
    for(int i=0;i<bestRes.netNodes.size();i++){
        if(bestRes.netNodes[i]){
            servers.push_back(i);
        }
    }
    /*
    for(auto e:servers){
        cout<<e<<" ";
    }
    cout<<endl;
    */
    vector<int> minCostPath[maxpath];
    int pathCnt=0;
    clock_t start=clock();
    int maxCap=graph.serverInfo[graph.serverInfo.size()-1].outCap;
    if(graph.nodeNum>=noRecv)
        res=zkw.multiMinCostFlow(servers,consumerNetNodes,flowNeed,maxCap,graph.nodeNum,minCostPath,pathCnt,false);
    else
        res=zkw.multiMinCostFlow(servers,consumerNetNodes,flowNeed,maxCap,graph.nodeNum,minCostPath,pathCnt,true);
    clock_t finish=clock();
    double time = (double)(finish-start)/CLOCKS_PER_SEC*1000;
    printf("zkw used time:%.1f ms\n",time);
    if(res.second==flowNeedAll){
        int minCost=res.first;
        int serverCost=0,layoutCost=0;
        int edgeSize=zkw.zkwEdges.size();
        for(int i=1,j=1;j<=servers.size();i+=2,j++){
            int curServerNode=servers[servers.size()-j];
            int curFlow=zkw.zkwEdges[edgeSize-i].flow;
            int serverID;
            serverID=findServerType(graph,-curFlow);//根据流量消耗判断服务器档次
            if(serverID==-1){
                cout<<"res error,serverNode:"<<curServerNode<<endl;
            }//某个服务器输出流量超过最高档次的服务器输出能力
            else{
                lastServers.insert(make_pair(curServerNode,serverID));
                //cout<<"serverID:"<<curServerNode<<" "<<serverID<<endl;
            }
            serverCost+=graph.serverInfo[serverID].cost;//加上该档次服务器的成本
            layoutCost+=graph.netCost[curServerNode];//加上服务器所在结点的部署成本
        }
        minCost+=serverCost;
        minCost+=layoutCost;
        lastMinCost=minCost;
        for(int i=0;i<maxpath;i++){
            vector<int> path;
            if(!minCostPath[i].empty()){
                for(auto node:minCostPath[i]){
                    path.push_back(node);
                    //cout<<node<<' ';
                }
                lastMinPath.push_back(path);
                //cout<<endl;
            }
        }
        cout<<"last cost:"<<lastMinCost<<endl;

/*
        //开始判断路径总费用是否与返回的cost一致
        int costSum=0,flowSum=0;
        vector<int> servers;
        for(auto path:lastMinPath){

            if(find(servers.begin(),servers.end(),path[0])==servers.end())
                servers.push_back(path[0]);
            int flow=path[path.size()-1];
            flowSum+=flow;
            int eachCostSum=0;
            for(int i=0;i<path.size()-2;i++){
                vector<Edge> curEdges=graph.G[path[i]];
                auto it=find_if(curEdges.begin(),curEdges.end(),EdgeFinder(path[i],path[i+1]));
                if(it!=curEdges.end()){
                    eachCostSum+=flow*(it->cost);
                }
            }
            costSum+=eachCostSum;
        }
        cout<<"flowAll:"<<flowSum<<endl;
        if(flowNeedAll!=flowSum)
            cout<<flowSum<<"!="<<flowNeedAll<<endl;
        else
            cout<<flowSum<<"=="<<flowNeedAll<<endl;
        if(costSum!=res.first)
            cout<<costSum<<"!="<<res.first<<" cost calculate error!"<<endl;
        else
            cout<<costSum<<"=="<<res.first<<" cost calculate right!"<<endl;
*/
    }
    else{
        cout<<"min path not found"<<endl;
    }

}
