import json
import io
import re
import os
import copy

umaLibraryPath = os.path.join(os.path.dirname(__file__), 'UmaMusumeLibrary_v2.json')
umaOldLibraryPath = os.path.join(os.path.dirname(__file__), 'UmaMusumeLibrary.json')

umaLibraryOriginPath = os.path.join(os.path.dirname(__file__), 'UmaMusumeLibraryOrigin.json')
umaLibraryModifyPath = os.path.join(os.path.dirname(__file__), 'UmaMusumeLibraryModify.json')

jsonOrigin = None
errorCount = 0
successCount = 0

def main():

    global jsonOrigin
    global errorCount
    global successCount

    print("選択肢に(成功)(失敗)の場合分けが入るようになってしまったので、こちら側で元に戻す処理を入れる")
    print('"Option": "成功",　"Option": "失敗", は消すだけでいいっぽい ((大成功) or (成功) も)')

    print("AddCharactorEvent はキャラのイベント名が存在しないときのみ追加")
    print("UpdateEvent はキャラのイベント名が存在する時のみ更新します")
    print("InterruptionEvent はキャラ名とイベント選択肢 部分一致で＜打ち切り＞を効果先頭に挿入")
    print("ReplaceEventName と ReplaceOption は完全一致で置換")
    print("ReplaceEffect は部分一致した分を置換します")

    jsonModify = None
    with io.open(umaLibraryOriginPath, 'r', encoding="utf-8") as f:
        print(f"json read: {umaLibraryOriginPath}")
        jsonOrigin = json.load(f)

    with io.open(umaLibraryModifyPath, 'r', encoding="utf-8") as f:
        print(f"json read: {umaLibraryModifyPath}")
        jsonModify = json.load(f)

    print("load complete")
    
    # "Option": "成功",　"Option": "失敗", を消す
    DeleteSuccessFailedOnly()

    # イベント名(成功), イベント名(失敗) を正規化
    NomarizeEventSuccessFailed()

    # AddCharactorEvent
    for charaEvent in jsonModify["Modify"]["AddCharactorEvent"]:
        for charaName, eventList in charaEvent.items():
            #print(f'{charaName}')
            for event in eventList["Event"]:
                AddCharactorEvent(charaName, event)

    # UpdateEvent
    for charaEvent in jsonModify["Modify"]["UpdateEvent"]:
        for charaName, eventList in charaEvent.items():
            #print(f'{charaName}')
            for event in eventList["Event"]:
                UpdateEvent(charaName, event)

    # InterruptionEvent
    for pair in jsonModify["Modify"]["InterruptionEvent"]:
        charaName = pair[0]
        optionText = pair[1]
        InterruptionEvent(charaName, optionText)

    # ReplaceEventName
    for replacePair in jsonModify["Modify"]["ReplaceEventName"]:
        searchText = replacePair[0]
        replaceText = replacePair[1]
        ReplaceEventName(searchText, replaceText)

    # ReplaceOption
    for replacePair in jsonModify["Modify"]["ReplaceOption"]:
        searchText = replacePair[0]
        replaceText = replacePair[1]
        ReplaceOption(searchText, replaceText)

    # ReplaceEffect
    for replacePair in jsonModify["Modify"]["ReplaceEffect"]:
        searchText = replacePair[0]
        replaceText = replacePair[1]
        ReplaceEffect(searchText, replaceText)


    # 旧バージョン向けのUmaMusumeLibraryを用意する
    ConvertOption3forOldVersion()

    print("\n\n")
    print("successCount: ", successCount)
    print("errorCount: ", errorCount)

    # jsonへ保存
    with io.open(umaLibraryPath, 'w', encoding="utf-8", newline='\n') as f:
        print(f"json wrtie: {umaLibraryPath}")
        json.dump(jsonOrigin, f, indent=2, ensure_ascii=False)

                    


def AddCharactorEvent(charaName, addEvent):
    print(f'AddCharactorEvent: {charaName}')
    global errorCount
    global successCount

    addCount = 0
    for prop, charaList in jsonOrigin["Charactor"].items():
        for orgCharaName, eventList in charaList.items():
            if charaName in orgCharaName:
                #print("jsonOriginからキャラ名が見つかったので、イベント名を探す")

                #print("同一イベントが存在するかどうか調べる")
                for addEventName, addEventList in addEvent.items():
                    for event in eventList["Event"]:
                        for eventName, eventOptionList in event.items():
                            #print(f'{eventName}')
                            if eventName == addEvent:
                                print(f'既に同じイベント名が存在します: {eventName}')
                                errorCount += 1
                                return False

                print(f"同一イベントが存在しなかったので、追加する: {addEventName}")
                eventList["Event"].append(addEvent)
                addCount += 1
                #return True    # 同キャラ名のイベントも探す

    if addCount > 0:
        successCount += addCount
        return True

    print("キャラが存在しませんでした")
    errorCount += 1
    return False

def UpdateEvent(charaName, updateEvent):
    print(f'UpdateEvent: {charaName}')
    global errorCount
    global successCount

    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                if charaName in orgCharaName:
                    #print("jsonOriginからキャラ名が見つかったので、イベント名を探す")

                    #print("同一イベントが存在するかどうか調べる")
                    for updateEventName, updateOptionList in updateEvent.items():
                        for event in eventList["Event"]:
                            for eventName, eventOptionList in event.items():
                                #print(f'{eventName}')
                                if eventName == updateEventName:
                                    print(f"イベント上書き: {eventName}")
                                    event[eventName] = updateEvent[eventName]
                                    successCount += 1
                                    return True     # とりあえず最初に発見したのだけ

                    print(f"同一イベントが存在しませんでした: {updateEvent}")
                    errorCount += 1
                    return False

    print("キャラが存在しませんでした")
    errorCount += 1
    return False

def InterruptionEvent(charaName, optionText):
    print(f'InterruptionEvent: [{charaName}, {optionText}]')
    global errorCount
    global successCount

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                if not (charaName in orgCharaName):
                    continue

                for event in eventList["Event"]:
                    newEvent = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')
                        for eventOption in eventOptionList:
                            if  optionText in eventOption["Option"]:
                                newEffectText = "<打ち切り>\n" + eventOption["Effect"]
                                eventOption["Effect"] = newEffectText
                                replaceCount += 1

    if replaceCount > 0:
        successCount += replaceCount
        return True

    print("optionTextが見つかりませんでした")
    errorCount += 1
    return False

def ReplaceEventName(searchText, replaceText):
    print(f'ReplaceEventName: [{searchText}, {replaceText}]')
    global errorCount
    global successCount

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    newEvent = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')
                        if eventName == searchText:
                            newEvent = {replaceText: eventOptionList}
                            break

                    if newEvent != None:
                        event.pop(eventName)
                        event.update(newEvent)

                        print(f"イベント名を置換: {orgCharaName} [{eventName}]->[{replaceText}]")
                        replaceCount += 1

    if replaceCount > 0:
        successCount += replaceCount
        return True

    print("serachTextが見つかりませんでした")
    errorCount += 1
    return False

def ReplaceOption(searchText, replaceText):
    print(f'ReplaceOption: [{searchText}, {replaceText}]')
    global errorCount
    global successCount

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    newEvent = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')
                        for eventOption in eventOptionList:
                            if eventOption["Option"] == searchText:
                                eventOption["Option"] = replaceText
                                print(f"選択肢名を置換: {orgCharaName} [{searchText}]->[{replaceText}]")
                                replaceCount += 1
                                break

    if replaceCount > 0:
        successCount += replaceCount
        return True

    print("serachTextが見つかりませんでした")
    errorCount += 1
    return False

def ReplaceEffect(searchText, replaceText):
    print(f'ReplaceEffect: [{searchText}, {replaceText}]')
    global errorCount
    global successCount

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    newEvent = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')
                        for eventOption in eventOptionList:
                            prevEffect = eventOption["Effect"]
                            newEffect = eventOption["Effect"].replace(searchText, replaceText)
                            if prevEffect != newEffect:
                                eventOption["Effect"] = newEffect
                                print(f"効果を置換: {orgCharaName} [{searchText}]->[{replaceText}]")
                                replaceCount += 1

    if replaceCount > 0:
        successCount += replaceCount
        return True

    print("serachTextが見つかりませんでした")
    errorCount += 1
    return False

def DeleteSuccessFailedOnly():
    print(f'DeleteSuccessFailedOnly')
    global errorCount
    global successCount

    deleteCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    deleteEventList = []
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')
                        if len(eventOptionList) != 2:
                            continue    # 選択肢が２個以外は見ない

                        op1 = eventOptionList[0]["Option"]
                        op2 = eventOptionList[1]["Option"]
                        if "成功" in op1 and "失敗" in op2:
                            deleteEventList.append(eventName)
                        elif ("大成功" in op1 and "成功" in op2) or ("成功" in op1 and "大成功" in op2):
                            deleteEventList.append(eventName)

                    for delEventName in deleteEventList:
                        print(f'deleteEvent: {delEventName}')
                        del event[delEventName]
                        deleteCount += 1

                # 空要素を削除する
                eventList["Event"] = [a for a in eventList["Event"] if a != {}]
                len(eventList["Event"])

    if deleteCount > 0:
        successCount += deleteCount
        return True

    print("(成功,失敗)が見つかりませんでした")
    errorCount += 1
    return False

def NomarizeEventSuccessFailed():
    print(f'NomarizeEventSuccessFailed')
    global errorCount
    global successCount

    rx1 = re.compile(r'(.+)(\(|（)成功(\)|）)')
    rx2 = re.compile(r'(.+)(\(|（)失敗(\)|）)')

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    newEvent = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')                        
                        newEventOptionList = []
                        optionSkip = False
                        replaced = False
                        for i in range(len(eventOptionList)):
                            if optionSkip:
                                optionSkip = False
                                continue

                            newEventOptionList.append(eventOptionList[i])

                            op1 = eventOptionList[i]["Option"]
                            ef1 = eventOptionList[i]["Effect"]
                            ret1 = rx1.match(op1)   # 選択肢に"成功"が入ってるか見る
                            if ret1 == None:
                                continue

                            if i + 1 == len(eventOptionList):
                                print("(失敗)が見つかりません")
                                errorCount += 1
                                break

                            op2 = eventOptionList[i + 1]["Option"]
                            ef2 = eventOptionList[i + 1]["Effect"]
                            ret2 = rx2.match(op2)
                            if ret2 == None:
                                print("(失敗)が見つかりません")
                                errorCount += 1
                                break

                            sub1 = ret1.group(1)
                            sub2 = ret2.group(1)
                            if sub1 != sub2:
                                print(f"イベント名が一致しません: {eventName}")
                                errorCount += 1
                                break

                            optionSkip = True
                            effectText = f"[成功]==========\n{ef1}\n[失敗]==========\n{ef2}"
                            newEventOptionList.pop(-1)
                            newEventOptionList.append({
                                "Option": sub1,
                                "Effect": effectText
                            })
                            replaced = True

                        if replaced:
                            newEvent = {eventName: newEventOptionList}
                        

                    if newEvent != None:
                        event.pop(eventName)
                        event.update(newEvent)

                        print(f"(成功/失敗)を置換: [{eventName}]")
                        replaceCount += 1

    if replaceCount > 0:
        successCount += replaceCount
        return True

    print("選択肢(成功),選択肢(失敗)が見つかりませんでした")
    errorCount += 1
    return False


def ConvertOption3forOldVersion():
    print(f'ConvertOption3forOldVersion')
    global errorCount
    global successCount

    jsonOldOrigin = copy.deepcopy(jsonOrigin)

    replaceCount = 0
    for charaOrSupport, propDict in jsonOldOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    newEvent = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')
                        if len(eventOptionList) >= 4:
                            print(f'4択選択肢を切り詰める: {eventName}')
                            eventOptionList.pop()
                            replaceCount += 1

    if replaceCount > 0:
        successCount += replaceCount

        # jsonへ保存
        with io.open(umaOldLibraryPath, 'w', encoding="utf-8", newline='\n') as f:
            print(f"json wrtie: {umaOldLibraryPath}")
            json.dump(jsonOldOrigin, f, indent=2, ensure_ascii=False)

        return True

    print("4択選択肢が見つかりませんでした")
    errorCount += 1
    return False


if __name__ == '__main__':
    print("start")
    main()
    print("finish")

