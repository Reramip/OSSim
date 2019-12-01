#include "manager.h"

const char* STATE_RUNNING = "running";
const char* STATE_READY = "ready";
const char* STATE_BLOCKED = "blocked";

// ���Ƚ��̣���RL��������ȼ����б��������ִ��
void Manager::schedule() {
    Process* toRunProc = nullptr;
    for (int i = 2; i >= 0; --i) {
        if (!rl[i].isEmpty()) {
            toRunProc = rl[i].getFirst();
            rl[i].removeFirst();
            break;
        }
    }
    if (toRunProc) {
        toRunProc->state = STATE_RUNNING;
        runProc = toRunProc;
    }
}

std::string Manager::allocate() {
    std::string result = "wake up: ";
    bool wakeup = false;
    for (int i = 0; i < 4; ++i) {
        if (res[i]->isWaiting()) {
            Process* reqProc = res[i]->waitingList.getFirst().reqProc;
            int reqCount = res[i]->waitingList.getFirst().reqCount;
            while (reqCount <= res[i]->idle) {
                // ���Ľ��̵���Դռ��
                Node<Process::UsingRes>* pBack = reqProc->usingResource.head;
                Node<Process::UsingRes>* pFront = pBack->next;
                bool flag = false;
                while (pFront) {
                    if (pFront->data.useRes == res[i]) {
                        pFront->data.useCount += reqCount;
                        flag = true;
                    }
                    if (flag) {
                        break;
                    }
                    pBack = pFront;
                    pFront = pFront->next;
                }
                if (!flag) {
                    reqProc->usingResource.addLast(Process::UsingRes(res[i], reqCount));
                }
                res[i]->idle -= reqCount;
                reqProc->waitingResCount--;


                if (!reqProc->isWaiting()) {    // ����������ȫ����Դ������
                    bl[reqProc->getPriority()].remove(reqProc);
                    rl[reqProc->getPriority()].addLast(reqProc);
                    reqProc->state = STATE_READY;
                    wakeup = true;
                    result += reqProc->getName() + " - ";
                    if (!runProc) {
                        reqProc->state = STATE_RUNNING;
                        runProc = reqProc;
                    }
                    else if (reqProc->getPriority() > runProc->getPriority()) {      // ��ռ
                        // ��ִ���еĽ��̸�Ϊ����̬
                        Process* toReadyProc = runProc;
                        toReadyProc->state = STATE_READY;
                        rl[toReadyProc->getPriority()].addLast(toReadyProc);
                        // �»��ѽ�����ռ
                        reqProc->state = STATE_RUNNING;
                        runProc = reqProc;
                    }
                }


                res[i]->waitingList.removeFirst();
                if (!res[i]->isWaiting()) {
                    break;
                }
                reqProc = res[i]->waitingList.getFirst().reqProc;
                reqCount = res[i]->waitingList.getFirst().reqCount;
            }
        }
    }
    if (wakeup) {
        result += "\b\b\b   ";
    }
    else {
        result += "no process waken up";
    }
    return result;
}

// �ӽ��̱����Ƴ����̣���RL/BL���Ƴ�����
// �ͷ���Դ������Դ�����б����Ƴ�����
// �ͷ��ڴ�
void Manager::kill(Process* killProc) {
    for (int i = 1; i < 3; ++i) {       // �ڽ��̱���ɾ��killProc
        bool flag = proc[i].remove(killProc);
        if (flag) {
            // �����ӽ���
            LinkedList<Process*>* children = &(killProc->child);
            while (!children->isEmpty()) {
                Process* killChild = children->getFirst();
                kill(killChild);
                children->removeFirst();
            }
            break;
        }
    }
    if (killProc->state == STATE_READY) {
        for (int i = 1; i < 3; ++i) {       // ��RL��ɾ��killProc
            bool flag = rl[i].remove(killProc);
            if (flag) {
                break;
            }
        }
    }
    else if (killProc->state == STATE_BLOCKED) {
        for (int i = 1; i < 3; ++i) {       // ��BL��ɾ��killProc
            bool flag = bl[i].remove(killProc);
            if (flag) {
                break;
            }
        }
        for (int i = 0; i < 4; ++i) {       // ����Դ�������ɾ��killProc
            if (res[i]->isWaiting()) {
                bool flag = res[i]->waitingList.remove(Resource::Request(killProc));
                if (flag) {
                    break;
                }
            }
        }
    }
    else if (killProc->state == STATE_RUNNING) {
        runProc = nullptr;
    }

    // �ͷ���Դ
    while (!killProc->usingResource.isEmpty()) {
        Resource* usingRes = killProc->usingResource.getFirst().useRes;
        int usingCount= killProc->usingResource.getFirst().useCount;
        killProc->usingResource.removeFirst();

        usingRes->idle += usingCount;
    }

    delete killProc;
}

Manager::Manager() {
    for (int i = 0; i < 4; ++i) {
        std::string str = "R" + std::to_string(i + 1);
        res[i] = new Resource(str, i + 1);
    }
}


Manager::~Manager() {
}

std::string Manager::getProcState(Process* p) {
    return "process " + p->getName() + " is " + p->state;
}

std::string Manager::init() {
    if (!proc[0].isEmpty()) {
        return "cannot init for multiple times";
    }
    std::string name = "init";
    Process* newProc = new Process(name, nullptr, 0);
    newProc->state = STATE_RUNNING;
    proc[0].addLast(newProc);
    runProc = newProc;
    return getProcState(runProc);
}

std::string Manager::create(std::string& name, int priority) {
    if (proc[0].isEmpty()) {
        return "must init first";
    }
    for (int i = 0; i < 3; ++i) {
        bool flag = false;
        if (!proc[i].isEmpty()) {
            Node<Process*>* p = proc[i].head->next;
            while (p) {
                if (p->data->getName() == name) {
                    flag = true;
                    break;
                }
                p = p->next;
            }
        }
        if (flag) {
            return "process " + name + " existed";
        }
    }
    // ��������
    Process* origProc = runProc;
    Process* newProc = new Process(name, runProc, priority);
    proc[priority].addLast(newProc);    // ��������б�

    newProc->state = STATE_READY;
    rl[priority].addLast(newProc);      // ����ready list

    runProc->child.addLast(newProc);    // Ϊ����������ӽ���
    // �½������ȼ�����ִ���еĽ��̣���ռ
    if (runProc->getPriority()<newProc->getPriority()) {
        // ��ִ���еĽ��̸�Ϊ����̬
        Process* toReadyProc = runProc;
        toReadyProc->state = STATE_READY;
   
        schedule();

        rl[toReadyProc->getPriority()].addLast(toReadyProc);

        return getProcState(runProc) + ".  " + getProcState(origProc);
    }

    return getProcState(runProc);
}

std::string Manager::destroy(std::string& name) {
    if (proc[0].isEmpty()) {
        return "must init first";
    }
    if (name == "init") {
        return "cannot destroy process init";
    }
    Process* deProc = nullptr;
    for (int i = 0; i < 3; ++i) {
        if (!proc[i].isEmpty()) {
            Node<Process*>* p = proc[i].head->next;
            while (p) {
                if (p->data->getName() == name) {
                    deProc = p->data;
                    break;
                }
                p = p->next;
            }
        }

        if (deProc) {         // �ҵ����̣�����ɱ���̲���
            // �Ӹý��̵ĸ����̵��ӽ����б���ȥ���ý���
            Process* father = deProc->getParent();
            if (father) {
                father->child.remove(deProc);
            }
            // �Ե�ǰ�����������ӽ��̼��ӽ���֮�ӽ��̣�
            //      �ӽ��̱����Ƴ�����RL/BL���Ƴ�
            //      �ͷ���Դ������Դ�������б���ɾ������
            kill(deProc);

            std::string alloRet = allocate();

            if (!runProc) {
                schedule();
            }

            return "process " + name + " destoryed.  " + getProcState(runProc) + ".  " + alloRet;
        }
    }
    return "process " + name + " does not exist";
}

std::string Manager::request(std::string& name, int count) {
    if (proc[0].isEmpty()) {
        return "must init first";
    }
    if (runProc->getName() == "init") {
        return "cannot request resources while init is running";
    }
    for (int i = 0; i < 4; ++i) {
        if (res[i]->getName() == name) {        // �ҵ���Դ���ж�����
            Process* reqProc = runProc;
            bool procChanged = false;
            if (res[i]->isWaiting() || count>res[i]->idle) {    // ��Դ�������������Դ����
                res[i]->waitingList.addLast(Resource::Request(runProc, count));
                runProc->waitingResCount++;
                bl[runProc->getPriority()].addLast(runProc);
                runProc->state = STATE_BLOCKED;
                schedule();
                return getProcState(runProc) + ".  " + getProcState(reqProc);
            }
            runProc->usingResource.addLast(Process::UsingRes(res[i], count));
            res[i]->idle -= count;

            return "process " + runProc->getName() + " requests " + std::to_string(count) + " " + res[i]->getName();
        }
    }
    return "resource " + name + " does not exist";
}

std::string Manager::release(std::string& name, int count) {
    if (proc[0].isEmpty()) {
        return "must init first";
    }
    for (int i = 0; i < 4; ++i) {
        if (res[i]->getName() == name) {        // �ҵ���Դ���ж��ͷ�
            std::string relProcName = runProc->getName();
            Node<Process::UsingRes>* pBack = runProc->usingResource.head;
            Node<Process::UsingRes>* pFront = pBack->next;
            while (pFront) {
                if (pFront->data.useRes->getName() == name) {
                    int legalCount = count > pFront->data.useCount ? pFront->data.useCount : count;     // ����ռ����������Ϊ���ֵ
                    pFront->data.useCount -= legalCount;
                    res[i]->idle += legalCount;
                    if (pFront->data.useCount == 0) {   // ��ȫ���ռ�ã�ɾ������usingResource�ж�Ӧ��
                        pBack->next = pFront->next;
                        runProc->usingResource.length--;
                        if (runProc->usingResource.isEmpty()) {
                            runProc->usingResource.tail = runProc->usingResource.head;
                        }
                    }
                    std::string alloRet = allocate();

                    std::string result = "process " + relProcName + " releases " + std::to_string(legalCount) + " " + name + " .  " + alloRet;
                    if (relProcName != runProc->getName()) {
                        result += "  " + getProcState(runProc);
                    }
                    return result;
                }
                pBack = pFront;
                pFront = pFront->next;
            }
            return "resource " + name + " have not been allocated to process " + runProc->getName();
        }
    }
    return "resource " + name + " does not exist";
}

std::string Manager::to() {
    if (proc[0].isEmpty()) {
        return "must init first";
    }
    Process* toReadyProc = runProc;

    bool flag = false;
    for (int i = 2; i > 0; --i) {
        if (!rl[i].isEmpty()) {
            flag = true;
            break;
        }
    }
    if (flag) {
        // ��ִ���еĽ��̸�Ϊ����̬
        toReadyProc->state = STATE_READY;
        schedule();
        rl[toReadyProc->getPriority()].addLast(toReadyProc);
        return getProcState(runProc) + ".  " + getProcState(toReadyProc);
    }
    return getProcState(runProc);
}

std::string Manager::listRL() {
    if (proc[0].isEmpty()) {
        return "must init first";
    }
    std::string result = "READY LIST:\n";
    for (int i = 2; i >= 0; --i) {
        result += "        " + std::to_string(i) + ":  ";
        if (!rl[i].isEmpty()) {
            Node<Process*>* pBack = rl[i].head;
            Node<Process*>* pFront = pBack->next;
            while (pFront) {
                result += pFront->data->getName() + " - ";
                pBack = pFront;
                pFront = pFront->next;
            }
            result += "\b\b\b   ";
        }
        result += "\n";
    }
    return result;
}

std::string Manager::listBL() {
    if (proc[0].isEmpty()) {
        return "must init first";
    }
    std::string result = "BLOCKED LIST:\n";
    for (int i = 0; i < 4; ++i) {
        result += "        " + res[i]->getName() + ": ";
        if (res[i]->isWaiting()) {
            Node<Resource::Request>* pBack = res[i]->waitingList.head;
            Node<Resource::Request>* pFront = pBack->next;
            while (pFront) {
                result += pFront->data.reqProc->getName() + " - ";
                pBack = pFront;
                pFront = pFront->next;
            }
            result += "\b\b\b   ";
        }
        result += "\n";
    }
    return result;
}

std::string Manager::listRes() {
    if (proc[0].isEmpty()) {
        return "must init first";
    }
    std::string result = "RESOURCES:\n";
    for (int i = 0; i <4; ++i) {
        result += "        " + res[i]->getName() + ": ";
        result += std::to_string(res[i]->idle);
        result += "\n";
    }
    return result;
}

std::string Manager::print(std::string& name) {
    for (int i = 0; i < 3; ++i) {
        bool flag = false;
        Node<Process*>* p = nullptr;
        if (!proc[i].isEmpty()) {
            p = proc[i].head->next;
            while (p) {
                if (p->data->getName() == name) {
                    flag = true;
                    break;
                }
                p = p->next;
            }
        }
        if (flag) {         // �ҵ����̣����д�ӡ����
            std::string result = "PROCESS  " + name + ":\n";
            result += "        using resources: ";
            if (!p->data->usingResource.isEmpty()) {
                Node<Process::UsingRes>* pBack = p->data->usingResource.head;
                Node<Process::UsingRes>* pFront = pBack->next;
                while (pFront) {
                    result += pFront->data.useRes->getName() + " - ";
                    pBack = pFront;
                    pFront = pFront->next;
                }
                result += "\b\b\b   ";
            }
            result += "\n";
            result += "        priority:        " + std::to_string(p->data->getPriority()) + "\n";
            result += "        state:           " + p->data->state + "\n";
            result += "        parent:          " + p->data->getParent()->getName() + "\n";
            result += "        children:        ";
            if (!p->data->child.isEmpty()) {
                Node<Process*>* pBack = p->data->child.head;
                Node<Process*>* pFront = pBack->next;
                while (pFront) {
                    result += pFront->data->getName() + " - ";
                    pBack = pFront;
                    pFront = pFront->next;
                }
                result += "\b\b\b   ";
            }
            result += "\n";
            return result;
        }
    }
    return "process not exist";
}
