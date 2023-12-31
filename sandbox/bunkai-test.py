import json
import os

# disable CUDA
os.environ["CUDA_VISIBLE_DEVICES"] = ""

from bunkai import Bunkai
from pathlib import Path

parentheses = [ ('<', '>'), ('"', '"'), ('「', '」')]

text="ウェブサイトのセキュリティ強化、ユーザーに安心してサイトを利用してもらうためのSSL暗号化(https化)ですが、みなさん対応は進んでいますか?\n2014年8月にGoogleウェブマスター向け公式ブログで「HTTPSをランキングシグナルに使用します」というアナウンスがあり、検索結果の順位付けの判定にSSL暗号化対応が導入されました。\nその後、2015 年12月に「HTTPS ページが優先的にインデックスに登録されるようになります」とSEOに有利になるアナウンスがあり、それから2年ほど経過し2018年2月に「保護されたウェブの普及を目指して」というタイトルのブログ記事が掲載されました。\n記事の内容は、2018年7月リリースのChrome 68からSSL暗号化対応を行っていないすべてのウェブサイトに「保護されていません」と表示する、というものです。\nこのコラムでは必要性や導入方法について説明していきますので参考にしてください。\nウェブサイトを閲覧する、お問い合わせフォームに個人情報を入力し、送信するなどWebクライアントとWebサーバとの間では常に重要な情報が送受信されています。SSL暗号化の対応を行っていないサイトでは、これらの重要な情報が第三者に盗み見られる(盗聴)、他人のID・パスワードを利用しウェブサイトを利用する(なりすまし)、Webサイトの情報が書き換えられる(改ざん)など多くのリスクがあります。\nSSL暗号化の対応を行うことでこういったリスクを減らすことができ、安心してユーザーにウェブサイトを利用してもらうことができるようになります。\nまた、先にも挙げた通り、SEO(検索結果の順位)にも多少なりとも効果が期待できます。\n1つ目が「共有SSL」です。レンタルサーバー会社が所有しているSSL化された空間の一部を借りる方法です。\n無料で簡単にSSL暗号化対応できることから、お問い合わせフォームなど特定のページだけSSL暗号化する場合に利用します。しかし、特定のページだけURLが変わってしまうなどのデメリットがあります。\n認証レベルには3種類あります。認証レベルはドメインの所有者をどこまで詳しく調べるかによって分けられます。下に行くほど認証レベルが高くなり、証明書の発行費用も高額になります。暗号の強度(どれだけ\n破られにくいか)は認証レベルとは関係ありません。\n認証レベル1:ドメイン認証・・・ドメインの持ち主であることを確認するSSLサーバ証明書(コーポレートサイト、店舗紹介サイトなど)\n最近では、無料で独自SSLが利用できるレンタルサーバーも増えてきています。まだ対応していなく迷っているウェブサイトの管理者は調べてみてください。\n当社のウェブサイトは今まで何もなかったから大丈夫だろう、そのうち対応すればよいだろうと考えているサイト管理者も、まだ多いと思いますが、実際に個人情報の漏洩やサイトの改ざんなどの被害が出てしまうと、企業としての信用も落としてしまうことになります。\nしくみがよくわからない、導入に不安という方は、専門家に相談してみるのも一つの方法です。\nまた、レンタルサーバー会社では、SSLサーバ証明書の設定を代行するサービスを行っている会社もありますので、問い合わせしてみてはいかがでしょうか?\nユーザーに安心してウェブサイトを利用してもらう、企業の信用を守るためにも、できるだけ早期のSSL暗号化の対応をお勧めします。\n<参考>ブラウザに下記のような表示がでます。(Chromeの場合)\nあなたがユーザーとしてウェブサイトを訪問し、問い合わせなどを行うのならどちらを選びますか?\n印刷・サイン業界で約15年、企業ブランドのイメージ戦略、販促支援などの企画営業を経験し、2009年に独立。神奈川県川崎市と福島県白河市を拠点に中小企業、特に小規模事業者を中心にIT導入からWebサイトの企画、構築、運営まで一貫して支援を行っています。\n\"ウェブサイトのSSL暗号化対応していますか?\"-を読んだ後におすすめの記事\nホーム > お役立ちコラム > ウェブサイトのSSL暗号化対応していますか?\n「ご飯を食べた。」と彼は言った "

bunkai = Bunkai(path_model=Path("bunkai-model-directory"))

lb_sep = '▁' #  # (U+2581)

tx = text.replace('\n', lb_sep)
#print(tx)

for sentence in bunkai(tx):
    print('sent', sentence)


