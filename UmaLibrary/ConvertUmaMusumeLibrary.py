from collections import OrderedDict
import json
import io
import re
import os
import copy

# 出力名など
umaLibraryName = 'UmaMusumeLibrary_v2.json'
umaOldLibraryName = 'UmaMusumeLibrary.json'
umaLibraryOriginName = 'UmaMusumeLibraryOrigin.json'
umaLibraryModifyName = 'UmaMusumeLibraryModify.json'

# 中断イベント一覧: https://gamerch.com/umamusume/entry/244582

# エラー発生時に中断させる
debugErrorStop = True

jsonOrigin = None
errorCount = 0
successCount = 0

def main(orgFolder):
    global jsonOrigin
    global errorCount
    global successCount

    if orgFolder == None:
        orgFolder = os.path.dirname(__file__)

    umaLibraryPath = os.path.join(orgFolder, umaLibraryName)
    umaOldLibraryPath = os.path.join(orgFolder, umaOldLibraryName)

    umaLibraryOriginPath = os.path.join(orgFolder, umaLibraryOriginName)
    umaLibraryModifyPath = os.path.join(orgFolder, umaLibraryModifyName)

    print("選択肢に(成功)(失敗)の場合分けが入るようになってしまったので、こちら側で元に戻す処理を入れる")
    print('"Option": "成功",　"Option": "失敗", は消すだけでいいっぽい ((大成功) or (成功) も)')

    print('スキル名に「」が付かなくなったので、元に戻す処理を入れる')
    print('イベント名から末尾の"①"や"（イベント進行①）"などの文字列を取り除く')

    print("AddCharactorEvent は キャラ/サポート のイベント名が存在しないときのみ追加")
    print("UpdateEvent はキャラのイベント名(完全一致)が存在する時のみ更新します")
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

    # AddCharactorEvent
    for charaEvent in jsonModify["Modify"]["AddCharactorEvent"]:
        for charaName, eventList in charaEvent.items():
            #print(f'{charaName}')
            for event in eventList["Event"]:
                AddCharactorEvent(charaName, event)
    if debugErrorStop and errorCount > 0:
        assert False

    # UpdateEvent
    for charaEvent in jsonModify["Modify"]["UpdateEvent"]:
        for charaName, eventList in charaEvent.items():
            #print(f'{charaName}')
            for event in eventList["Event"]:
                UpdateEvent(charaName, event)
    if debugErrorStop and errorCount > 0:
        assert False

    # "Option": "成功",　"Option": "失敗", を消す
    DeleteSuccessFailedOnly()
    if debugErrorStop and errorCount > 0:
        assert False

    # 効果の正規化
    NormalizeEffect()

    # 大成功 / 成功 (/ 失敗) のみの選択肢をイベントリストから削除する
    NomarizeEventSuperSuccessSuccessFailed()
    if debugErrorStop and errorCount > 0:
        assert False

    # イベント名(成功), イベント名(失敗) を正規化
    NomarizeEventSuccessFailed()
    if debugErrorStop and errorCount > 0:
        assert False

    # スキル名に「」が付かなくなったので、元に戻す処理を入れる
    NormarizeSkillName()
    if debugErrorStop and errorCount > 0:
        assert False

    # イベント名から末尾の"①"や"（イベント進行①）"などの文字列を取り除く
    EraseEventNameSuffixMetaData()
    if debugErrorStop and errorCount > 0:
        assert False

    # InterruptionEvent
    for pair in jsonModify["Modify"]["InterruptionEvent"]:
        charaName = pair[0]
        optionText = pair[1]
        InterruptionEvent(charaName, optionText)
    if debugErrorStop and errorCount > 0:
        assert False

    # ReplaceEventName
    for replacePair in jsonModify["Modify"]["ReplaceEventName"]:
        searchText = replacePair[0]
        replaceText = replacePair[1]
        ReplaceEventName(searchText, replaceText)
    if debugErrorStop and errorCount > 0:
        assert False

    # ReplaceOption
    for replacePair in jsonModify["Modify"]["ReplaceOption"]:
        searchText = replacePair[0]
        replaceText = replacePair[1]
        ReplaceOption(searchText, replaceText)
    if debugErrorStop and errorCount > 0:
        assert False

    # ReplaceEffect
    for replacePair in jsonModify["Modify"]["ReplaceEffect"]:
        searchText = replacePair[0]
        replaceText = replacePair[1]
        ReplaceEffect(searchText, replaceText)
    if debugErrorStop and errorCount > 0:
        assert False

    # 旧バージョン向けのUmaMusumeLibraryを用意する
    ConvertOption3forOldVersion(umaOldLibraryPath)

    print("\n\n")
    print("successCount: ", successCount)
    print("errorCount: ", errorCount)

    # jsonへ保存
    with io.open(umaLibraryPath, 'w', encoding="utf-8", newline='\n') as f:
        print(f"json wrtie: {umaLibraryPath}")

        SortCharaNameDict(jsonOrigin)
        json.dump(jsonOrigin, f, indent=2, ensure_ascii=False)


# キャラ名やサポート名でソートする
def SortCharaNameDict(jsonOrigin):
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            sorted_charaList = OrderedDict(sorted(charaList.items(),  key=lambda x:x[0]))
            del propDict[prop]
            propDict[prop] = sorted_charaList
            

def AddCharactorEvent(charaName, addEvent):
    print(f'AddCharactorEvent: {charaName}')
    global errorCount
    global successCount

    addCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
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

    ############################################
    print("キャライベントを新規追加します")
    eventArray = [addEvent]
    jsonOrigin["Support"]["SSR"].update({charaName: {"Event": eventArray}})

    successCount += 1
    return True

    errorCount += 1
    return False


def UpdateEvent(charaName, updateEvent):
    print(f'UpdateEvent: {charaName}')
    global errorCount
    global successCount

    updateCount = 0

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
                                if updateEventName in eventName:    # イベント名は部分一致でも可 (origin側に"①"とかついてるかもしれないし)
                                    print(f"イベント上書き: {eventName}")
                                    event[eventName] = updateEvent[updateEventName]
                                    successCount += 1
                                    updateCount += 1
                                    #return True     # とりあえず最初に発見したのだけ

    if updateCount > 0:
        return True
    else :
        print(f"同一イベントが存在しませんでした: {updateEvent}")
        errorCount += 1
        return False

    # print("キャラが存在しませんでした")
    # errorCount += 1
    # return False


def EraseEventNameSuffixMetaData():
    print(f'EraseEventNameSuffixMetaData: ')
    global errorCount
    global successCount

    rx = re.compile(r'( ?(①|②|③|④)|（.+(①|②|③|④)）)$')

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    newEvent = None
                    newEventName = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')
                        replaceResult = rx.subn('', eventName)
                        if replaceResult[1] > 0:
                            newEventName = replaceResult[0]
                            newEvent = {newEventName: eventOptionList}
                            break

                    if newEvent != None:
                        event.pop(eventName)
                        event.update(newEvent)

                        print(f"イベント名を置換: {orgCharaName} [{eventName}]->[{newEventName}]")
                        replaceCount += 1

    if replaceCount > 0:
        successCount += replaceCount
        return True

    print("serachTextが見つかりませんでした")
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
                            newEffect = (eventOption["Effect"] + "\n").replace(searchText, replaceText).strip()
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
                        elif "成功" in op1 and "成功" in op2:
                            deleteEventList.append(eventName)
                        elif eventName == "":   # 空イベント
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


def NormalizeEffect():
    print(f'NormalizeEffect')
    global successCount

    rxPlus = re.compile(r'[+＋]')
    rxMinus = re.compile(r'[-－‐-]')
    rxTilde = re.compile(r'[~～]')
    rxRange = re.compile(r'([+-])(\d+～)(\d+)')
    rxEnum = re.compile(r'([+-])(\d+\/)(\d+)')
    rxCircle = re.compile(r'[○◯〇]')
    rxHintLevel = re.compile(r'ヒント(?:lv|レベル)', re.I)
    rxLevel = re.compile(r'lv\+?(\d+)', re.I)
    rxSkillPoint = re.compile(r'スキルPt', re.I)
    rxLigature = re.compile(r'絆(?:(?:ゲージ)?\+(\d)?|ゲージ(?:Lv\+1)?$)')
    rxLineHead1 = re.compile(r'^([※])\s+')
    rxLineHead2 = re.compile(r'^([└∟])\s*')

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    for eventName, eventOptionList in event.items():
                        for eventOption in eventOptionList:
                            op1 = eventOption["Option"]
                            ef1 = eventOption["Effect"]
                            effectLineList = ef1.split('\n')
                            for i in range(len(effectLineList)):
                                effectLine = effectLineList[i]
                                replacedText = effectLine.strip()
                                replacedText = rxPlus.sub(r'+', replacedText)
                                replacedText = rxMinus.sub(r'-', replacedText)
                                replacedText = rxTilde.sub(r'～', replacedText)
                                replacedText = rxRange.sub(r'\1\2\1\3', replacedText)
                                replacedText = rxEnum.sub(r'\1\2\1\3', replacedText)
                                replacedText = rxCircle.sub(r'〇', replacedText)
                                replacedText = rxHintLevel.sub(r'ヒントLv', replacedText)
                                replacedText = rxLevel.sub(r'Lv+\1', replacedText)
                                replacedText = rxSkillPoint.sub(r'スキルPt', replacedText)
                                replacedText = rxLigature.sub(r'絆ゲージ+\1', replacedText)
                                replacedText = rxLineHead1.sub(r'\1', replacedText)
                                replacedText = rxLineHead2.sub(r'└', replacedText)
                                if replacedText != effectLine:
                                    print(f'"{effectLine}" -> "{replacedText}"')
                                    effectLineList[i] = replacedText
                                    replaceCount += 1

                            skillReplacedEffect = '\n'.join(effectLineList)
                            eventOption["Effect"] = skillReplacedEffect

    successCount += replaceCount
    return True


# 大成功 / 成功 (/ 失敗) のみの選択肢をイベントリストから削除する
def NomarizeEventSuperSuccessSuccessFailed():
    print(f'NomarizeEventSuperSuccessSuccessFailed')
    global errorCount
    global successCount

    rx1 = re.compile(r'(.*?)(\(|（|【)?大成功(\)|）|】)?')
    rx2 = re.compile(r'(.*?)(\(|（|【)?成功(\)|）|】)?')
    rx3 = re.compile(r'(.*?)(\(|（|【)?失敗(\)|）|】)?')

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    newEvent = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')
                        
                        # 選択肢が、大成功 / 成功 (/ 失敗) かどうか調べる
                        assert(len(eventOptionList) >= 2)
                        op1 = eventOptionList[0]["Option"]
                        ef1 = eventOptionList[0]["Effect"]
                        ret1 = rx1.match(op1)   # 選択肢1 に"大成功"が入ってるか
                        if ret1 != None:
                            op2 = eventOptionList[1]["Option"]
                            ef2 = eventOptionList[1]["Effect"]
                            ret2 = rx2.match(op2)
                            if ret2 != None:    # 選択肢2 に"成功"が入ってるか
                                # 選択肢が同一か調べる
                                sub1 = ret1.group(1)
                                sub2 = ret2.group(1)
                                if sub1 == sub2:
                                    if len(eventOptionList) == 2:
                                        # 大成功 / 成功 のみなので、イベントをリストから削除
                                        event.pop(eventName)
                                        print(f"(大成功/成功)を削除: [{eventName}]")
                                        replaceCount += 1
                                        break

                                    # (失敗) がある場合は、そちらも調べる
                                    if len(eventOptionList) == 3:
                                        op3 = eventOptionList[2]["Option"]
                                        ef3 = eventOptionList[2]["Effect"]
                                        ret3 = rx3.match(op3)
                                        if ret3 != None:
                                            sub3 = ret3.group(1)
                                            if sub2 == sub3:
                                                # 大成功 / 成功 / 失敗 のみなので、イベントをリストから削除
                                                event.pop(eventName)
                                                print(f"(大成功/成功/失敗)を削除: [{eventName}]")
                                                replaceCount += 1
                                                break


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
                            ret1 = rx1.match(op1)   # 選択肢に"大成功"が入ってるか見る
                            if ret1 == None:
                                continue

                            if i + 1 == len(eventOptionList):
                                print("(成功)が見つかりません")
                                errorCount += 1
                                break

                            op2 = eventOptionList[i + 1]["Option"]
                            ef2 = eventOptionList[i + 1]["Effect"]
                            ret2 = rx2.match(op2)
                            if ret2 == None:
                                # 単に選択肢に"大成功"が入ってるだけのパターン
                                break

                            sub1 = ret1.group(1)
                            sub2 = ret2.group(1)
                            if sub1 != sub2:
                                print(f"選択肢が一致しません: {eventName}")
                                errorCount += 1
                                break

                            optionSkip = True
                            effectText = f"[大成功]==========\n{ef1}\n[成功]==========\n{ef2}"
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

                        print(f"(大成功/成功)を置換: [{eventName}]")
                        replaceCount += 1

                    # 空要素を削除する
                    eventList["Event"] = [a for a in eventList["Event"] if a != {}]

    if replaceCount > 0:
        successCount += replaceCount
        return True

    print("選択肢(成功),選択肢(失敗)が見つかりませんでした")
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
                                # (成功) / (成功) パターンなら置換しない
                                if  rx1.match(op2):
                                    break

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


def NormarizeSkillName():
    print(f'NormarizeSkillName')
    global errorCount
    global successCount

    rxSkillHint = re.compile(r'(\S+) (Lv\+\d)')
    rxSkillHint2 = re.compile(r'^(?:([^「」]*?)「([^「」]+?)」|([^「」]+?))(の?)(?:ヒントLv|ヒント|Lv)(\+\d)(?:を獲得)?')

    replaceCount = 0
    for charaOrSupport, propDict in jsonOrigin.items():
        for prop, charaList in propDict.items():
            for orgCharaName, eventList in charaList.items():
                for event in eventList["Event"]:
                    newEvent = None
                    for eventName, eventOptionList in event.items():
                        #print(f'{eventName}')

                        for eventOption in eventOptionList:
                            op1 = eventOption["Option"]
                            ef1 = eventOption["Effect"]
                            effectLineList = ef1.split('\n')
                            for i in range(len(effectLineList)):
                                effectLine = effectLineList[i]
                                replacedText = rxSkillHint.sub(r'「\1」\2', effectLine)
                                replacedText = rxSkillHint2.sub(r'\1「\2\3」\4ヒントLv\5', replacedText)
                                if replacedText != effectLine:
                                    print(f'"{effectLine}" -> "{replacedText}"')
                                    effectLineList[i] = replacedText
                                    replaceCount +=  1

                            skillReplacedEffect = '\n'.join(effectLineList)
                            eventOption["Effect"] = skillReplacedEffect


    if replaceCount > 0:
        successCount += replaceCount
        return True

    print("[スキル名 Lv+\d] が見つかりませんでした")
    errorCount += 1
    return False


def ConvertOption3forOldVersion(umaOldLibraryPath):
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

            SortCharaNameDict(jsonOldOrigin)
            json.dump(jsonOldOrigin, f, indent=2, ensure_ascii=False)

        return True

    print("4択選択肢が見つかりませんでした")
    errorCount += 1
    return False


if __name__ == '__main__':
    print("start")
    main(None)
    print("finish")
