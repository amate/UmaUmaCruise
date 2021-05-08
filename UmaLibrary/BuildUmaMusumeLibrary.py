import json
import io
import re
import os
import time

debug = False

umaLibraryPath = os.path.join(os.path.dirname(__file__), 'UmaMusumeLibrary.json')

umaLibraryOriginPath = os.path.join(os.path.dirname(__file__), 'UmaMusumeLibraryOrigin.json')
umaLibraryModifyPath = os.path.join(os.path.dirname(__file__), 'UmaMusumeLibraryModify.json')

jsonOrigin = None
errorCount = 0
successCount = 0

def main():

    global jsonOrigin
    global errorCount
    global successCount

    print("AddCharactorEvent はキャラのイベント名が存在しないときのみ追加")
    print("UpdateEvent はキャラのイベント名が存在する時のみ更新します")
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

    print("\n\n")
    print("successCount: ", successCount)
    print("errorCount: ", errorCount)

    # jsonへ保存
    with io.open(umaLibraryPath, 'w', encoding="utf-8") as f:
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

    for prop, charaList in jsonOrigin["Charactor"].items():
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

                print(f"同一イベントが存在しませんでした: {addEventName}")
                errorCount += 1
                return False

    print("キャラが存在しませんでした")
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











if __name__ == '__main__':
    print("start")
    if not debug:
        main()
    else:
        debug_main()
    print("finish")

