#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>
#include "time.h"

#include "ga.h"
#include "zkw.h"
#include "isap.h"

using namespace std;

extern mt19937  mt;
//根据fitness比较
bool cmpByFitness1(const Individual &someone1,const Individual &someone2){//升序
    return someone1.fitness<someone2.fitness;
}
bool cmpByFitness2(const Individual &someone1,const Individual &someone2){//降序
    return someone1.fitness>someone2.fitness;
}

//根据流量消耗查询所属服务器档次，返回档次ID
int ATCGA::findServerType(const Graph &graph,int flow)
{
    for(int i=0;i<graph.serverInfo.size();i++){
        if(flow<=graph.serverInfo[i].outCap)
            return i;
    }
    return -1;
}

//计算个体的适应度并写入到该个体
bool ATCGA::calFitness(const Graph &graph,const vector<int> &consumerNetNodes,vector<int> &flowNeed,
                         int flowNeedAll,Individual &someone)
{ 
/*
    ZKW_MinCost zkw;
    if(graph.nodeNum>=noRecv)
        zkw.createZKW(graph,false);
    else
        zkw.createZKW(graph,true);
*/

    pair<int,int> res;
    vector<int> servers;
    for(int i=0;i<someone.gene.size();i++){
        if(someone.gene[i])servers.push_back(i);
    }

    ISAP isap;
    isap.createISAP(graph,servers);

    /*
    if(graph.nodeNum>=noRecv)
        res=zkw.multiMinCostFlow(servers,consumerNetNodes,flowNeed,maxCap,graph.nodeNum,false);
    else
        res=zkw.multiMinCostFlow(servers,consumerNetNodes,flowNeed,maxCap,graph.nodeNum,true);
    */
    res=isap.multiMinCostFlow(graph.nodeNum,graph.nodeNum+1,graph.nodeNum+2);
    if(res.second==flowNeedAll){
        int minCost=res.first;
        int serverCost=0,layoutCost=0;
        int edgeSize=isap.Edges.size();
        for(int i=2,j=1;j<=servers.size();i+=2,j++){
            int curServerNode=servers[servers.size()-j];
            int curFlow=isap.Edges[edgeSize-i].flow;
            int serverID;
            serverID=findServerType(graph,curFlow);//根据流量消耗判断服务器档次
            if(serverID==-1)return false;//某个服务器输出流量超过最高档次的服务器输出能力
            serverCost+=graph.serverInfo[serverID].cost;//加上该档次服务器的成本
            layoutCost+=graph.netCost[curServerNode];//加上服务器所在结点的部署成本
        }
        minCost+=serverCost;
        minCost+=layoutCost;
        someone.fitness=(double)minCost;//基本适应度为minCost
        return true;
    }
    return false;//非有效个体（不是可行解）
}

//计算当代种群的平均适应度并写入到成员
double ATCGA::calCurAvgFitness()
{
    double sumFitness=0,avgFitness;
    for(int i=0;i<curPopulation.size();i++){
        sumFitness+=curPopulation[i].fitness;
    }
    avgFitness=sumFitness/curPopulation.size();
    curAvgFitness=avgFitness;
    return avgFitness;
}

//计算当代种群的最优适应度并写入到成员(值越小适应度越优)
double ATCGA::calCurBestFitness()
{
    curBestFitness=INF;
    for(size_t i=0;i<curPopulation.size();i++){
        if(curPopulation[i].fitness<curBestFitness){
            curBestFitness=curPopulation[i].fitness;
        }
    }
    return curBestFitness;
}

//计算当代种群的最差适应度
double ATCGA::calWorstFitness()
{
    curWorstFitness=0;
    for(size_t i=0;i<curPopulation.size();i++){
        if(curPopulation[i].fitness>curWorstFitness)
            curWorstFitness=curPopulation[i].fitness;
    }
    return curWorstFitness;
}

//计算当代种群的适应度分布值并写入数据成员
double ATCGA::calFitnessDistribution()
{
    curFitnessDist=(curWorstFitness-curBestFitness)/curAvgFitness;
    fitnessDists.push_back(curFitnessDist);
    return curFitnessDist;
}

//计算当代种群与父代种群的代沟值并写入数据成员
double ATCGA::calGenerationGap()
{
    double preBestFitness=0.0,preAvgFitness=0.0;
    double sumFitness=0.0;
    for(size_t i=0;i<prePopulation.size();i++){
        sumFitness+=prePopulation[i].fitness;
    }
    preAvgFitness=sumFitness/curPopulation.size();
    preBestFitness=0;
    for(size_t i=0;i<prePopulation.size();i++){
        if(prePopulation[i].fitness>preBestFitness)
            preBestFitness=prePopulation[i].fitness;
    }
    curGenerationGap=(preBestFitness-curBestFitness)+(preAvgFitness-curAvgFitness);
    generationGaps.push_back(curGenerationGap);
    return curGenerationGap;
}

//生成初始种群
double ATCGA::createFirstPopulation(const Graph &graph,int randIndividualNum)
{
    vector<int> consumerNetNodes,flowNeed;
    int flowNeedAll=0;
    for(int i=0;i<graph.consumerNum;i++){
        consumerNetNodes.push_back(graph.consumers[i].netNode);
        flowNeed.push_back(graph.consumers[i].flowNeed);
        flowNeedAll+=graph.consumers[i].flowNeed;
    }

    //原始人出现了(初始解：服务器全部放消费结点邻近的网络结点)
    int noFirstFlag=0;
    vector<int> firstGene(graph.nodeNum,0);
    for(int i=0;i<graph.consumerNum;i++){
        firstGene[graph.consumers[i].netNode]=1;
    }
    Individual Lucy;//传说地球上的第一个灵猿叫露西
    Lucy.gene=firstGene;
    int minCost=0;
    for(int i=0;i<graph.consumerNum;i++){
        int curServerNode=graph.consumers[i].netNode;
        int serverID=findServerType(graph,graph.consumers[i].flowNeed);
        if(serverID==-1){
            noFirstFlag=1;
            cout<<"no first res!"<<endl;
            goto RAND;//直连解不存在，开始随机生成
        }
        minCost+=graph.serverInfo[serverID].cost;
        minCost+=graph.netCost[curServerNode];
    }
    Lucy.fitness=minCost;
    curPopulation.push_back(Lucy);
    bestIndividual=Lucy;
    firstIndividual=Lucy;


RAND:
    double time=0.0,d=0.0;
    clock_t start=clock();
    //然后上帝又产生了一批新的随机个体
    if(randIndividualNum){
        int numCnt=randIndividualNum;
        if(noFirstFlag)numCnt+=1;
        int num=0;
        int downRand=graph.consumerNum/2;
        int upRand=graph.consumerNum-1;
        clock_t d_start=clock();
        while(numCnt){
            ZKW_MinCost zkw;

            if(graph.nodeNum>=noRecv)
                zkw.createZKW(graph,false);
            else
                zkw.createZKW(graph,true);
            vector<int> gene(geneLen,0);

            if(d>1&&upRand+100<graph.nodeNum&&graph.nodeNum>smallcase){//d=2 upRand+=50
                d_start=clock();
                d=0.0;
                upRand+=100;//每隔1秒找不到初始种群，就扩大随机搜索范围
            }
            num=rand()%(upRand-downRand+1)+downRand;
            int connectDirectNum,restNum;
            if(graph.nodeNum>smallcase){
                connectDirectNum=num*0.8;
                restNum=num-connectDirectNum;
                for(int j=0;j<connectDirectNum;j++){//一部分服务器结点直连
                    int consumerIndex=rand()%graph.consumerNum;
                    int serverNode=graph.consumers[consumerIndex].netNode;
                    gene[serverNode]=1;
                }
                for(int j=0;j<restNum;j++){//一部分服务器随机生成
                    int serverNode=rand()%graph.nodeNum;
                    gene[serverNode]=1;
                }
            }
            else{
                for(int j=0;j<num;j++){//初中级直接随机生成
                    int serverNode=rand()%graph.nodeNum;
                    gene[serverNode]=1;
                }
            }

            Individual someone;
            someone.gene=gene;
            if(!calFitness(graph,consumerNetNodes,flowNeed,flowNeedAll,someone)){
                //cout<<"someone is not a human"<<endl;//随机产生了非可行解，石头什么的不能遗传！
                //someone.fitness=INF;
                //curPopulation.push_back(someone);
                //numCnt--;
            }
            else{
                if(noFirstFlag){
                    noFirstFlag=0;
                    firstIndividual=someone;//最后一个随机生成的个体作为固定初始解
                }
                if(someone.fitness<bestIndividual.fitness){
                    bestIndividual=someone;//随机个体中最优的个体作为初始最优解
                }
                curPopulation.push_back(someone);
                numCnt--;
            }
            clock_t d_finish=clock();
            d = (double)(d_finish-d_start)/CLOCKS_PER_SEC;
        }

    }

    FOUND:
    if(curPopulation.size()!=popsize){
        cout<<"popsize error"<<endl;//未找到足够的可行解
    }
    //初始种群诞生了，设定每个个体的选择概率(用于轮盘赌)
    double sumFitness=0;
    for(size_t i=0;i<curPopulation.size();i++){
        sumFitness+=1/(curPopulation[i].fitness);//待优化，可做一定尺度变换加大适应度差距后再求概率
    }
    for(int i=0;i<popsize;i++){
        double selectRate=1/(curPopulation[i].fitness)/sumFitness;
        curPopulation[i].selectRate=selectRate;
    }
    calCurAvgFitness();
    calCurBestFitness();
    calWorstFitness();
    calFitnessDistribution();
    prePopulation=curPopulation;

    clock_t finish=clock();
    time = (double)(finish-start)/CLOCKS_PER_SEC;
    cout<<"create population used time:"<<time<<endl;
    return time;
}

//选择方案1:竞争选择（两代竞争算法）
void ATCGA::selectByCompete(vector<pair<Individual, Individual> > &parents, int parentsNum)
{

    //两代遗传，保留优秀基因
    vector<Individual> mergePopulation(popsize*2);
    sort(curPopulation.begin(),curPopulation.end(),cmpByFitness1);
    if(prePopulation.size()!=0){
        sort(prePopulation.begin(),prePopulation.end(),cmpByFitness1);
        set_union(curPopulation.begin(),curPopulation.end(),prePopulation.begin(),prePopulation.end(),
              mergePopulation.begin(),cmpByFitness1);
        copy(mergePopulation.begin(),mergePopulation.end()-mergePopulation.size()/2,curPopulation.begin());//取前合并后的一半
    }

    for(int i=0;i<parentsNum;i++){//找parentsNum对父母对
        int k=rand()%(popsize-1)+2;//[2,popsize]
        vector<Individual> temp;
        for(int j=0;j<k;j++){
            int randIndex=rand()%popsize;//[0,popsize-1]
            temp.push_back(curPopulation[randIndex]);
        }
        sort(temp.begin(),temp.end(),cmpByFitness1);
        parents.push_back(make_pair(temp[0],temp[1]));//取适应度最优的两个
    }
}

//选择方案2：轮盘赌选择（两代比例选择算法）
void ATCGA::selectByProportion(vector<pair<Individual, Individual> > &parents, int parentsNum)
{
    //两代遗传，保留优秀基因
    vector<Individual> mergePopulation(popsize*2);
    sort(curPopulation.begin(),curPopulation.end(),cmpByFitness1);
    if(prePopulation.size()!=0){//初始种群无父代
        sort(prePopulation.begin(),prePopulation.end(),cmpByFitness1);
        merge(curPopulation.begin(),curPopulation.end(),prePopulation.begin(),curPopulation.end(),
              mergePopulation.begin(),cmpByFitness1);
        copy(mergePopulation.begin(),mergePopulation.end()-mergePopulation.size()/2,curPopulation.begin());//取前合并后的一半
    }

    for(int k=0;k<parentsNum;k++){//找parentsNum对父母对
        int m=0;
        int randNum1=rand()%1001;//[0,1000]
        int randNum2=rand()%1001;//[0,1000]
        pair<Individual,Individual> parent;
        int cnt=0;
        for(int i=0;i<popsize;i++){
            m+=curPopulation[i].selectRate*1000;
            if(randNum1<m||randNum2<m){
               if(randNum1<m){
                   parent.first=curPopulation[i];
                   cnt++;
               }
               if(randNum2<m){
                   parent.second=curPopulation[i];
                   cnt++;
               }
               if(cnt==2)break;
            }
        }
        if(parent.first.gene!=parent.second.gene){
            parents.push_back(parent);
        }
        else k--;//自己跟自己不能繁殖。。
    }
}

//选择方案3:竞争选择（每次只选一个）
void ATCGA::selectOnlyOneByCompete(Individual &someone)
{

    //两代遗传，保留优秀基因
    vector<Individual> mergePopulation(popsize*2);
    sort(curPopulation.begin(),curPopulation.end(),cmpByFitness1);
    if(prePopulation.size()!=0){
        sort(prePopulation.begin(),prePopulation.end(),cmpByFitness1);
        set_union(curPopulation.begin(),curPopulation.end(),prePopulation.begin(),prePopulation.end(),
              mergePopulation.begin(),cmpByFitness1);
        copy(mergePopulation.begin(),mergePopulation.end()-mergePopulation.size()/2,curPopulation.begin());//取前合并后的一半
    }

    int k=rand()%popsize+1;//[1,popsize]
    vector<Individual> temp;
    for(int j=0;j<k;j++){
        int randIndex=rand()%popsize;//[0,popsize-1]

        temp.push_back(curPopulation[randIndex]);
    }
    auto it=min_element(temp.begin(),temp.end(),cmpByFitness1);
    someone=*it;//取适应度最优
}

//选择方案4:轮盘赌选择（每次只选一个）
void ATCGA::selectOnlyOneByProportion(Individual &someone)
{
    //两代遗传，保留优秀基因
    vector<Individual> mergePopulation(popsize*2);
    sort(curPopulation.begin(),curPopulation.end(),cmpByFitness1);
    if(prePopulation.size()!=0){
        sort(prePopulation.begin(),prePopulation.end(),cmpByFitness1);
        set_union(curPopulation.begin(),curPopulation.end(),prePopulation.begin(),prePopulation.end(),
              mergePopulation.begin(),cmpByFitness1);
        copy(mergePopulation.begin(),mergePopulation.end()-mergePopulation.size()/2,curPopulation.begin());//取前合并后的一半
    }

    int m=0;
    int randNum=rand()%1001;//[0,1000]
    for(int i=0;i<popsize;i++){
        m+=curPopulation[i].selectRate*1000;
        if(randNum<m){
            someone=curPopulation[i];
            break;
        }
    }
}

//交叉方案1：单点交叉(淘汰)
void ATCGA::crossOverByOneBit(const pair<Individual, Individual> &parent,
                              pair<Individual, Individual> &children)
{
    int randBit=rand()%(geneLen);//[0,geneLen]
    int randBitLen=rand()%(geneLen-randBit)+1;
    //int randBitLen=1;
    children.first.gene=parent.first.gene;
    children.second.gene=parent.second.gene;
    for(int i=randBit;i<randBit+randBitLen;i++){
        children.first.gene[i]=parent.second.gene[i];
        children.second.gene[i]=parent.first.gene[i];
    }
}

//交叉方案2:随机多点交叉
void ATCGA::crossOverByRandBits(const pair<Individual, Individual> &parent,
                                pair<Individual, Individual> &children)
{
    //根据规模调整交叉点数
    int downRand=5,upRand=10;
    double k1=0.1,k2=0.4;//0.2 0.4
    downRand=downRand*(geneLen/50+1)*k1;
    upRand=upRand*(geneLen/50+1)*k2;

    int randBitsNum=rand()%(upRand-downRand+1)+downRand;
    vector<int> childTemp1=parent.first.gene,childTemp2=parent.second.gene;
    for(int i=0;i<randBitsNum;i++){
        int randBit=rand()%(parent.first.gene.size());
        //int randBitLen=rand()%(geneLen-randBit)+1;(随机多点多位淘汰)
        int randBitLen=1;//(随机多点单位case3不行)
        children.first.gene=childTemp1;
        children.second.gene=childTemp2;
        for(int i=randBit;i<randBit+randBitLen;i++){
            children.first.gene[i]=parent.second.gene[i];
            children.second.gene[i]=parent.first.gene[i];
        }
        childTemp1=children.first.gene;//保存前一次交叉后的基因结果
        childTemp2=children.second.gene;
    }
}

//交叉方案3:自适应多点交叉，先评估当前迭代次数下适应度分布，后确定交叉的位数
void ATCGA::crossOverByAutoBits(const pair<Individual, Individual> &parent,
                                pair<Individual, Individual> &children)
{
    //根据规模调整交叉点数
    int downRand=5,upRand=10;
    double k1=0.1,k2=0.3;
    downRand=downRand*(geneLen/50+1)*k1;
    upRand=upRand*(geneLen/50+1)*k2;
    double sumFitnessDist=0.0,avgFitnessDist;
    for(size_t i=0;i<fitnessDists.size();i++){
        sumFitnessDist+=fitnessDists[i];
    }
    avgFitnessDist=sumFitnessDist/fitnessDists.size();

    if(curFitnessDist<avgFitnessDist*0.8){//小于平均多样性的80%，交叉点数范围扩大
        int randBitsNum=rand()%((int)(upRand*1.4)-downRand+1)+downRand;
        vector<int> childTemp1=parent.first.gene,childTemp2=parent.second.gene;
        for(int i=0;i<randBitsNum;i++){
            int randBit=rand()%(geneLen);
            //int randBitLen=rand()%(geneLen-randBit)+1;
            int randBitLen=1;
            children.first.gene=childTemp1;
            children.second.gene=childTemp2;
            for(int i=randBit;i<randBit+randBitLen;i++){
                children.first.gene[i]=parent.second.gene[i];
                children.second.gene[i]=parent.first.gene[i];
            }
            childTemp1=children.first.gene;//保存前一次交叉后的基因结果
            childTemp2=children.second.gene;
        }
    }
    else if(curFitnessDist<avgFitnessDist*0.9){//小于平均多样性水平90%
        int randBitsNum=rand()%((int)(upRand*1.2)-downRand+1)+downRand;
        vector<int> childTemp1=parent.first.gene,childTemp2=parent.second.gene;
        for(int i=0;i<randBitsNum;i++){
            int randBit=rand()%(parent.first.gene.size());
            //int randBitLen=rand()%(geneLen-randBit)+1;
            int randBitLen=1;
            children.first.gene=childTemp1;
            children.second.gene=childTemp2;
            for(int i=randBit;i<randBit+randBitLen;i++){
                children.first.gene[i]=parent.second.gene[i];
                children.second.gene[i]=parent.first.gene[i];
            }
            childTemp1=children.first.gene;//保存前一次交叉后的基因结果
            childTemp2=children.second.gene;
        }
    }
    else{//基本达到平均水平
        int randBitsNum=rand()%(upRand-downRand+1)+downRand;
        vector<int> childTemp1=parent.first.gene,childTemp2=parent.second.gene;
        for(int i=0;i<randBitsNum;i++){
            int randBit=rand()%(parent.first.gene.size());
            //int randBitLen=rand()%(geneLen-randBit)+1;
            int randBitLen=1;
            children.first.gene=childTemp1;
            children.second.gene=childTemp2;
            for(int i=randBit;i<randBit+randBitLen;i++){
                children.first.gene[i]=parent.second.gene[i];
                children.second.gene[i]=parent.first.gene[i];
            }
            childTemp1=children.first.gene;//保存前一次交叉后的基因结果
            childTemp2=children.second.gene;
        }
    }
}

//交叉方案4:每次与固定个体交叉
void ATCGA::crossOverWithFixed(const Individual &someone,pair<Individual, Individual> &children,bool crossFlag)
{  
    vector<int> childTemp1=someone.gene;

    vector<int> childTemp2;
    if(crossFlag)
        childTemp2=firstIndividual.gene;//与初始解交叉
    else
        childTemp2=bestIndividual.gene;//与最优解交叉

    int randBit1,randBit2;
    //找到两个服务器的基因位置
    do{
        randBit1=rand()%(geneLen);
    }
    while(childTemp1[randBit1]==0);
    do{
        randBit2=rand()%(geneLen);
    }
    while(childTemp2[randBit2]==0);

    children.first.gene=childTemp1;
    children.second.gene=childTemp2;

    //交换
    children.first.gene[randBit1]=0;
    children.second.gene[randBit1]=1;
    children.first.gene[randBit2]=1;
    children.second.gene[randBit2]=0;

/*
    vector<int> childTemp2=bestIndividual.gene;//与最优的交叉
    //多点+与最优解
    int downRand=5,upRand=10;
    double k1=0.1,k2=0.4;//0.2 0.4
    downRand=downRand*(geneLen/50+1)*k1;
    upRand=upRand*(geneLen/50+1)*k2;

    //test
    //int downRand=1;
    //int upRand=1;
    int randBitsNum=rand()%(upRand-downRand+1)+downRand;

    for(int i=0;i<randBitsNum;i++){
        int randBit=rand()%(geneLen);

        children.first.gene=childTemp1;
        children.second.gene=childTemp2;
        children.first.gene[randBit]=bestIndividual.gene[randBit];
        children.second.gene[randBit]=someone.gene[randBit];
        childTemp1=children.first.gene;//保存前一次交叉后的基因结果
        childTemp2=children.second.gene;
    }
*/

}

//变异方案1：单点单位变异
void ATCGA::mutateByOneBit(Individual &someone)
{

    double mutateRand=rand()%1001;//[0,1000]
    if(mutateRand<mutationRate*1000){
        int randBit=rand()%(geneLen);
        //uniform_int_distribution<int> dist(0,geneLen-1);
        //int randBit=dist(mt);

        int randBitLen=1;
        for(int i=randBit;i<randBit+randBitLen;i++){
            if(someone.gene[i])someone.gene[i]=0;
            else someone.gene[i]=1;
        }
    }
}

//变异方案2:自适应单点变异，每次先评估代沟计算变异率后再变异(有严重bug待修正)
void ATCGA::mutateByAutoBits(Individual &someone)
{
    double sumFitnessDist=0.0,avgFitnessDist;
    double sumGenerationGap=0.0,avgGenerationGap=0.0;
    for(size_t i=0;i<generationGaps.size();i++){
        sumGenerationGap+=generationGaps[i];
    }
    avgGenerationGap=sumGenerationGap/generationGaps.size();
    for(size_t i=0;i<fitnessDists.size();i++){
        sumFitnessDist+=fitnessDists[i];
    }
    avgFitnessDist=sumFitnessDist/fitnessDists.size();

    if(sumGenerationGap>0.0){//存在代沟，自适应调整变异率
        //cout<<mutationRate*(3/(1+curGenerationGap/avgGenerationGap
        //                       +curFitnessDist/avgFitnessDist))<<endl;
        mutationRate=mutationRate*(3/(1+curGenerationGap/avgGenerationGap
                                      +curFitnessDist/avgFitnessDist));
        int mutateRand=rand()%1001;//[0,1000]
        //自适应调整变异率：
        if(mutateRand<mutationRate*1000){
            int randBit=rand()%(geneLen);
            int randBitLen=rand()%(geneLen-randBit)+1;
            for(int i=randBit;i<randBit+randBitLen;i++){
                if(someone.gene[i])someone.gene[i]=0;
                else someone.gene[i]=1;
            }
        }
        mutationRate=0.2;//回到基准值
    }
    else{
        int mutateRand=rand()%1001;//[0,1000]
        //按基本变异率执行
        if(mutateRand<mutationRate*1000){
            int randBit=rand()%(geneLen);
            int randBitLen=rand()%(geneLen-randBit)+1;
            for(int i=randBit;i<randBit+randBitLen;i++){
                if(someone.gene[i])someone.gene[i]=0;
                else someone.gene[i]=1;
            }
        }
    }

}

//种群更新:每次种群有变化时调用此函数更新数据成员
void ATCGA::updatePopulation()
{
    //新种群诞生了，设定每个个体的选择概率(用于轮盘赌)
    double sumFitness=0;
    for(size_t i=0;i<curPopulation.size();i++){
        sumFitness+=1/(curPopulation[i].fitness);//待优化，可做一定尺度变换加大适应度差距后再求概率
    }
    for(int i=0;i<popsize;i++){
        double selectRate=1/(curPopulation[i].fitness)/sumFitness;
        curPopulation[i].selectRate=selectRate;
    }
    calCurAvgFitness();
    calCurBestFitness();
    calWorstFitness();
    calFitnessDistribution();
    calGenerationGap();
    prePopulation=curPopulation;
}

//追加变异:超级变异
void ATCGA::superMutation(Individual &someone)
{
    //直接变异
    int randBit=rand()%geneLen;
    int randBitLen=rand()%(geneLen-randBit)+1;
    for(int i=randBit;i<randBit+randBitLen;i++){
        if(someone.gene[i])someone.gene[i]=0;
        else someone.gene[i]=1;
    }
}

//演化：迭代times次，将结果写入最佳个体（服务器布局）
double ATCGA::evolve(const Graph &graph)
{

    clock_t timeout_start=clock();
    clock_t d_start=clock();
    vector<int> consumerNetNodes,flowNeed;
    int flowNeedAll=0;
    for(int i=0;i<graph.consumerNum;i++){
        consumerNetNodes.push_back(graph.consumers[i].netNode);
        flowNeed.push_back(graph.consumers[i].flowNeed);
        flowNeedAll+=graph.consumers[i].flowNeed;
    }
    //创建初始种群
    double createTimeUsed=createFirstPopulation(graph,2);

    //int parentsNum=2;//每次选出1对
    //迭代：
    int noNicerCnt1=0,noNicerCnt2=0,smallQuitCnt=0,middleQuitCnt=0;
    double noNicerTime=0.0,noNicerFlag=0;
    int nicerDist=0;
    vector<Individual> newIndividuals;

    for(int i=0;i<times;i++){
        Individual someone;
        pair<Individual,Individual> children;

        selectOnlyOneByCompete(someone);
        //selectOnlyOneByProportion(someone);

        if(!someone.gene.empty()){
            //clock_t curFinish=clock();
            //double curCrossTime = (double)(curFinish-timeout_start)/CLOCKS_PER_SEC;
            crossOverWithFixed(someone,children,false);
            //crossOverWithFixed(someone,children,true);
            if(!children.first.gene.empty())
                mutateByOneBit(children.first);//单点变异
            if(!children.second.gene.empty())
                mutateByOneBit(children.second);
        }

        if(calFitness(graph,consumerNetNodes,flowNeed,flowNeedAll,children.first)){
            newIndividuals.push_back(children.first);
        }
        if(calFitness(graph,consumerNetNodes,flowNeed,flowNeedAll,children.second)){
            newIndividuals.push_back(children.second);
        }

        if(!newIndividuals.empty()){
            auto it=min_element(newIndividuals.begin(),newIndividuals.end(),cmpByFitness1);
            //演化初期，更优解与当前最优解的差值较大，随后降低

            //控制全局搜索粒度，从而控制遗传收敛速度
/*
            if(geneLen>=bigcase){
                clock_t curFinish=clock();
                double curTime = (double)(curFinish-timeout_start)/CLOCKS_PER_SEC;
                if(curTime<10){
                    nicerDist=2000;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1800)nicerDist-=30;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=10&&curTime<13){
                    nicerDist=1800;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1600)nicerDist-=30;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=13&&curTime<16){
                    nicerDist=1600;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1500)nicerDist-=30;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=16&&curTime<19){
                    nicerDist=1500;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1400)nicerDist-=30;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=19&&curTime<22){
                    nicerDist=1400;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1200)nicerDist-=20;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=22&&curTime<25){
                    nicerDist=1200;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1000)nicerDist-=20;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=25&&curTime<28){
                    nicerDist=1000;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>800)nicerDist-=20;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=28&&curTime<31){
                    nicerDist=800;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>700)nicerDist-=20;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=31&&curTime<34){
                    nicerDist=700;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>600)nicerDist-=10;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=34&&curTime<37){
                    nicerDist=600;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>500)nicerDist-=10;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=37&&curTime<40){
                    nicerDist=500;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>400)nicerDist-=10;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=40&&curTime<45){
                     nicerDist=400;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>300)nicerDist-=10;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=45&&curTime<50){
                    nicerDist=300;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>200)nicerDist-=5;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=50&&curTime<55){
                    nicerDist=200;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>150)nicerDist-=5;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=55&&curTime<60){
                    nicerDist=150;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>120)nicerDist-=5;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=60&&curTime<65){
                    nicerDist=120;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>80)nicerDist-=5;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=65&&curTime<70){
                    nicerDist=80;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>50)nicerDist-=2;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=70&&curTime<80){
                    nicerDist=50;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>30)nicerDist-=2;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else nicerDist=10;
            }

            if(geneLen>smallcase&&geneLen<bigcase){
                clock_t curFinish=clock();
                double curTime = (double)(curFinish-timeout_start)/CLOCKS_PER_SEC;
                if(curTime<10){
                    nicerDist=1500;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1200)nicerDist-=30;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=10&&curTime<13){
                    nicerDist=1200;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1100)nicerDist-=30;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=13&&curTime<16){
                    nicerDist=1100;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>1000)nicerDist-=30;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=16&&curTime<19){
                    nicerDist=1000;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>900)nicerDist-=30;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=19&&curTime<22){
                    nicerDist=900;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>800)nicerDist-=20;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=22&&curTime<25){
                    nicerDist=800;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>700)nicerDist-=20;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=25&&curTime<28){
                    nicerDist=700;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>600)nicerDist-=20;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=28&&curTime<31){
                    nicerDist=600;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>500)nicerDist-=20;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=31&&curTime<34){
                    nicerDist=500;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>400)nicerDist-=10;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=34&&curTime<37){
                    nicerDist=400;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>300)nicerDist-=10;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=37&&curTime<40){
                    nicerDist=300;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>250)nicerDist-=10;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=40&&curTime<45){
                     nicerDist=250;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>200)nicerDist-=10;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=45&&curTime<50){
                    nicerDist=200;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>150)nicerDist-=5;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=50&&curTime<55){
                    nicerDist=150;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>120)nicerDist-=5;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=55&&curTime<60){
                    nicerDist=120;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>100)nicerDist-=5;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=60&&curTime<65){
                    nicerDist=100;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>80)nicerDist-=5;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=65&&curTime<70){
                    nicerDist=80;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>50)nicerDist-=2;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else if(curTime>=70&&curTime<80){
                    nicerDist=50;
                    if(noNicerFlag){
                        noNicerFlag=0;
                        if(nicerDist>30)nicerDist-=2;//每隔0.2秒找不到更优解，减小noNicerDist
                    }
                }
                else nicerDist=10;
            }
*/

            //把适应度最优的加入种群
            if(it->fitness<bestIndividual.fitness-nicerDist){//比最优的更优，加进种群
                cout<<i<<"find better:"<<it->fitness<<' '<<nicerDist<<endl;
                bestIndividual=*it;
                auto iter=max_element(curPopulation.begin(),curPopulation.end(),cmpByFitness1);//找出最差的个体
                *iter=bestIndividual;//用最优的替换
                updatePopulation();//更新种群
                noNicerCnt1=0;
                noNicerCnt2=0;
                d_start=clock();
            }
            else{
                noNicerCnt1++;
                noNicerCnt2++;
                smallQuitCnt++;
                middleQuitCnt++;
                clock_t d_finish=clock();
                noNicerTime = (double)(d_finish-d_start)/CLOCKS_PER_SEC;
                if(noNicerTime>0.5){
                    noNicerFlag=1;
                    noNicerTime=0.0;
                }

            }
        }


TIME_OUT:

        //超时判断
        clock_t timeout_finish=clock();
        double timeout = (double)(timeout_finish-timeout_start)/CLOCKS_PER_SEC;
        if(geneLen<=smallcase&&timeout>smallMaxTime){//初级每个种群演化时间不能超过17s(5个种群)
            cout<<i<<"time out:"<<timeout<<endl;
            return timeout;
        }
        if(geneLen>smallcase&&geneLen<bigcase&&timeout>middleMaxTime){//中级每个种群演化时间不能超过43s(2个种群)
            cout<<i<<"time out:"<<timeout<<endl;
            return timeout;
        }
        if(geneLen>=bigcase&&timeout>bigMaxTime){//高级种群演化时间不能超过88s
            cout<<i<<"time out:"<<timeout<<endl;
            return timeout;
        }
/*
        if(geneLen<bigcase&&geneLen>smallcase&&bestIndividual.fitness<250000){
            return timeout;
        }
        if(geneLen>=bigcase&&bestIndividual.fitness<400000){
            return timeout;
        }
*/
        //迭代准备
        //parents.clear();
        newIndividuals.clear();
    }
    return 0.0;
}

//求解：演化完成后取出最佳个体bestIndividual计算结果(返回lastMinCost以及lastMinPath)
void ATCGA::getResult(const Individual &bestIndividual,const Graph &graph,
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
    for(int i=0;i<geneLen;i++){
        if(bestIndividual.gene[i]){
            servers.push_back(i);
        }
    }
    /*
    for(auto e:servers){
        cout<<e<<" ";
    }
    */
    cout<<endl;
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

