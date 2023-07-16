import spacy
nlp = spacy.load('ja_ginza_electra')


text="""生活用品・インテリア・雑貨 (業務用000セット) ジョインテックス 両面テープ 00mm×00m b000j [×000セット]
 --floridadailychronicle.com\n[送料無料]柱上安全帯用ベルト(スライドバックルタイプ) 黒 マーベル mat-000wb_okrjs
 本間ゴルフ lb-000 ドライバー lb-0000 カーボンシャフト [honma golf ホンマ driver] ?長い\nmedomalk メドーマルク
 s0-00s ゲートタイプ 車止め ステンレス製 φ00.0 差込式 受注生産 上 strymon / riverside multistage drive [オーバ
ードライブ/ディストーション] リバーサイド ストライモン\nオーハウス電子てんびん(天秤)pa0000jp カスタムオーダー[
0000年モデル]フォーティーン fourteencf-000 フェアウェイウッドディアマナ wカーボンシャフトdiamana w ■taiyo 高性
能油圧シリンダ〔品番:000h-00ca00bb000-ab-yl〕[tr-0000000][個人宅配送不可][個人宅と代引不可 運賃別途お見積り]\n
花王エスト est エターナルフローローション エンリッチド [国内向正規品] [送料無料]車いす用スロープ 段ない・ス フ
レックスタイプ 0000 000-000 シコク w0000 まるい c-000 トレー ワゴン クリアグリーン\n[インターセントラル]ベース
ボードヒーター自然対流方式ベースボード型電気暖房器ehsシリーズ床置壁固定タイプサーモスタット別売・ブラケット付
属単相000v/0.0kwehc-0000 [人気の花柄]テイコブ おしゃれな横押しキャリーカート [おとなりカート ブレーキ付き ベーシックタイプ] wcc00 ※右用のみ※ ブラック/ネイビー 保冷バッグ0way 幸和製作所[送料無料]ショッピングカート シルバ
ーカート シニア プレゼント キャリー 0輪 平田なるみ [送料無料]吸い殻入れ ys-000 黒 [代引不可]\n[送料無料][無料
健康相談 対象製品]ブルークロス救急蘇生セット (一般救急用) エマジンバッグシリーズ arw-0 [高度管理] ?長い\nお取
り寄せ お取り寄せ nba セルティックス ショートパンツ/ショーツ チェッカーボード スウィングマン ミッチェル&ネス/mitchell & ness ブラック 映画\"スノーホワイト\" スノーホワイト 衣装,コスチューム 大人女性用 独特\ndaiwaトランク大将 su-0000x ブラック /メーカー[ダイワ daiwa グローブライド globeride)][送料無料] ラブライブ!サンシャイン!! aqoursメンバーウィッグ 桜内梨子 コスプレウィッグ [ パーティーグッズ かぶりもの コスプレ アニメ かつら キャラク
ターウィッグ ら! カツラ 髪の毛 ラ! ハロウィン 衣装 ゲーム 漫画 変装グッズ ラ! プチ仮装 ] ?新しい\nmizuno ミズ
ノ ロジンバッグ 00個入り yamaha -ヤマハ- rmx リミックス 000 ドライバー tour ad vr-0 シャフト[0000年モデル][smtb-ms]\nroddio (ロッディオ) amorphous slim shaft/リシャフト工賃込中古 cランク (フレックスr) タイトリスト vokey spin milled sm0 ツアークロム 00°/00°f 純正特注シャフト r 男性用 右利き ウェッジ wg ボーケイ スピンミルド 中古
ゴルフクラブ second hand 大きい\nrawlings リバティ advanced 00\"\" softball catcher's mitt - right ハンド スルー (海外取寄せ品)中古 cランク (フレックスsr) キャロウェイ legacy black(0000) 000 0.0° speed metalix zx(ドライ
バー) sr 男性用 右利き ドライバー dr レガシー ブラック カーボン 中古ゴルフクラブ second hand 罰金\n[国内未発売]ボタフォゴ ホーム 半袖 00番 ジョルジ・ワグネル[0000/botafogo/ブラジルリーグ/サッカー]0000 シマノ ボーダレス
並継キャスティング仕様 000h0[大型商品][送料無料] 東京都"""

text="西田幾多郎は、自分の生い立ちを振り返ってみて、自分の人生の「不幸」は、自分の「我」が「大我」と「超我」の間にある、つまり自分は「小我」に過ぎなかったことに気づかされたのですね。 つまり、「小我」から解放されることによって、生きること、そして「生きる意味」に目覚めることができるというのです。 つまり、人としての「生」における「意味」は、人が「小我」から"


text="ですです"


def char_is_hiragana(c):
    return u'\u3040' <= c <= u'\u309F'

def contains_hiragana(s):
    return any(char_is_hiragana(c) for c in s)

doc = nlp(text)
#print(doc)

for sent in doc.sents:

    if not contains_hiragana(sent.text):
        continue

    has_adp = False
    has_aux = False
    has_noun = False
    all_aux = True

    for token in sent:
        print(token)
        print(token.pos_)

        if not token.pos_:
            all_aux = False

        if token.pos_ == 'ADP':
            has_adp = True
        elif token.pos_ == 'AUX':
            has_aux = True
        elif token.pos_ in ('NOUN', 'PRON', 'PROPN'):
            has_noun = True

    if (not has_adp) or (not has_aux):
        #remove
        pass
    else:
        print(sent)

    if all_aux:
        print("all aux")

