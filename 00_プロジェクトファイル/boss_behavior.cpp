//=============================================
//
//ボスのストラテジーパターン[enemy_behavior.cpp]
//Auther Matsuda Towa
//
//=============================================
#include "boss_behavior.h"
#include "wave_boss.h"
#include "player.h"
#include "boss_enemy.h"
#include "block.h"
#include "object.h"
#include "smoke_range.h"
#include "block_piece.h"
#include "tackle_charge.h"

//=============================================
//コンストラクタ
//=============================================
CBossWandering::CBossWandering():m_MoveIdx(INT_ZERO), m_StopCnt(INT_ZERO), m_isMove()
{
	m_isMove = true;
}

//=============================================
//デストラクタ
//=============================================
CBossWandering::~CBossWandering()
{
}

//=============================================
//徘徊
//=============================================
void CBossWandering::Wandering(CBossEnemy* boss)
{
	if (m_isMove)
	{
		//移動先の情報取得(引数が移動先のポイントの配列番号)
		CMovePoint* pMovePoint = CWave_Boss::GetMovePoint(m_MoveIdx);

		//対象の位置への方向情報
		D3DXVECTOR3 point = { pMovePoint->GetPos().x - boss->GetPos().x,FLOAT_ZERO,pMovePoint->GetPos().z - boss->GetPos().z };

		// 目的地との距離を計算
		float distance = sqrtf(point.x * point.x + point.z * point.z);

		// まだ目的地に到達していない場合のみ移動処理を行う
		if (distance > THRESHOLD)
		{
			//対象物との角度計算
			float angle = atan2f(point.x, point.z);

			D3DXVECTOR3 move = boss->GetMove();

			move.x += sinf(angle) * boss->GetSpeed();
			move.z += cosf(angle) * boss->GetSpeed();
			//親クラスからrotを取得
			D3DXVECTOR3 rot = boss->GetRot();
			rot.y = angle + D3DX_PI;
			//rotを代入
			boss->SetRot(rot);
			//移動量代入
			boss->SetMove(move);

			switch (boss->GetAxis())
			{
			case CBossEnemy::X:
				PickNextMovePoint(pMovePoint);
				break;
			case CBossEnemy::Z:
				PickNextMovePoint(pMovePoint);
				break;
			default:
				break;
			}

			boss->SetMotion(CBossEnemy::MOTION_MOVE);
		}
		else
		{//到達していたら
			m_isMove = false;

			D3DXVECTOR3 move = VEC3_RESET_ZERO;
			boss->SetMove(move);

			//次の移動先の抽選
			PickNextMovePoint(pMovePoint);

			boss->SetMotion(CBossEnemy::MOTION_NEUTRAL);

		}
	}
	else if (!m_isMove)
	{//動かない状態なら
		//指定フレーム分止まる
		StopCnt();
	}
}

//=============================================
//指定フレーム分止まる
//=============================================
void CBossWandering::StopCnt()
{
	++m_StopCnt;
	if (m_StopCnt > STOP_FRAME)
	{
		m_StopCnt = INT_ZERO;
		m_isMove = true;
	}
}

//=============================================
//次の移動先の抽選
//=============================================
void CBossWandering::PickNextMovePoint(CMovePoint* pMovePoint)
{
	std::random_device seed;
	std::mt19937 random(seed());
	std::uniform_int_distribution<int> number(INT_ZERO, pMovePoint->GetNumPoint());
	//ランダムで位置指定
  	m_MoveIdx = number(random);
	if (m_MoveIdx >= pMovePoint->GetNumPoint())
	{
		m_MoveIdx = INT_ZERO;
	}
}

//=============================================
//徘徊のデバッグ表示
//=============================================
void CBossWandering::DrawDebug()
{
#ifdef _DEBUG
	LPD3DXFONT pFont = CManager::GetInstance()->GetRenderer()->GetFont();
	RECT rect = { 0,0,SCREEN_WIDTH,SCREEN_HEIGHT };
	char aStr[256];

	sprintf(&aStr[0], "\n[ボス進む位置]%d"
		,m_MoveIdx );
	//テキストの描画
	pFont->DrawText(NULL, &aStr[0], -1, &rect, DT_RIGHT, D3DCOLOR_RGBA(255, 0, 0, 255));
#endif // _DEBUG
}

//=============================================
//コンストラクタ
//=============================================
CBossChase::CBossChase():m_bTargetPlayer(true)
{
}

//=============================================
//デストラクタ
//=============================================
CBossChase::~CBossChase()
{
}

//=============================================
//追跡
//=============================================
void CBossChase::Chase(CBossEnemy* boss, CObject* obj)
{
	CPlayer* pplayer = dynamic_cast<CPlayer*>(obj);

	//プレイヤーの位置への方向情報
	D3DXVECTOR3 Vector = pplayer->GetPos() - boss->GetPos();
	// 目的地との距離を計算
	float distance = sqrtf(Vector.x * Vector.x + Vector.z * Vector.z);

	//プレイヤーに向かって動かす
	MovetoPlayer(distance, THRESHOLD, Vector, boss);

	D3DXVec3Normalize(&Vector, &Vector);

	// レイキャストを実行し、障害物があるか判定
	if (boss->PerformRaycast_Smoke(Vector, boss).hit)
	{
		m_bTargetPlayer = false;

		boss->ChangeState(new CConfusionBossState);
		return;
	}
	else if (boss->PerformRaycast_Block(Vector, boss).hit
		&&boss->PerformRaycast_Player(Vector, boss).distance > boss->PerformRaycast_Block(Vector, boss).distance)
	{
		m_bTargetPlayer = false;
		boss->ChangeState(new CSearchState);
	}
	else
	{
		m_bTargetPlayer = true;
	}
}

//=============================================
//プレイヤーに向かって動かす
//=============================================
void CBossChase::MovetoPlayer(float distance, const float& threshold, D3DXVECTOR3& Vector, CBossEnemy* boss)
{
	//対象物との角度計算
	float angle = atan2f(Vector.x, Vector.z);
	// まだ目的地に到達していない場合のみ移動処理を行う
	if (distance > threshold)
	{
		D3DXVECTOR3 move = boss->GetMove();

		move.x += sinf(angle) * boss->GetSpeed();
		move.z += cosf(angle) * boss->GetSpeed();
		//移動量代入
		boss->SetMove(move);
	}
	else
	{
		D3DXVECTOR3 move = VEC3_RESET_ZERO;
		//移動量代入
		boss->SetMove(move);
	}

	//親クラスからrotを取得
	D3DXVECTOR3 rot = boss->GetRot();
	rot.y = angle + D3DX_PI;
	//rotを代入
	boss->SetRot(rot);
}

//=============================================
//追跡のデバッグ表示
//=============================================
void CBossChase::DrawDebug()
{
#ifdef _DEBUG
	LPD3DXFONT pFont = CManager::GetInstance()->GetRenderer()->GetFont();
	RECT rect = { 0,0,SCREEN_WIDTH,SCREEN_HEIGHT };
	char aStr[256];
	if (m_bTargetPlayer)
	{
		sprintf(&aStr[0], "\n\n\n\n対象:プレイヤー");
	}
	else if (!m_bTargetPlayer)
	{
		sprintf(&aStr[0], "\n\n\n\n対象:プレイヤー以外");
	}
	//テキストの描画
	pFont->DrawText(NULL, &aStr[0], -1, &rect, DT_RIGHT, D3DCOLOR_RGBA(255, 0, 0, 255));
#endif // _DEBUG
}

//=============================================
//コンストラクタ
//=============================================
CBossConfusion::CBossConfusion(): m_isRight(false), m_TurnCnt(0)
{
}

//=============================================
//デストラクタ
//=============================================
CBossConfusion::~CBossConfusion()
{
}

//=============================================
//混乱状態
//=============================================
void CBossConfusion::Confusion(CBossEnemy* boss, float StartRot_y)
{
	//現在の方向を取得
	D3DXVECTOR3 rot = boss->GetRot();

	//移動量を算出
	float Rot_Answer_y = rot.y - StartRot_y;
	
	//方向移動処理
	MoveRot(rot, Rot_Answer_y, boss);

	//自分の方向を取得
	D3DXVECTOR3 vec = { sinf(boss->GetRot().y + D3DX_PI), FLOAT_ZERO, cosf(boss->GetRot().y + D3DX_PI)};

	D3DXVec3Normalize(&vec, &vec);

	CCharacter::RayHitInfo HitPlayerInfo = PerformRaycast_Player(vec, boss);
	CCharacter::RayHitInfo HitBlockInfo = boss->PerformRaycast_Block(vec, boss);
	CCharacter::RayHitInfo HitSmokeInfo = boss->PerformRaycast_Smoke(vec, boss);

	if (HitPlayerInfo.hit && HitPlayerInfo.distance < HitBlockInfo.distance)
	{//見つけたら
		//boss->ChangeState(new CChaseState);
	}
	if (m_TurnCnt >= NUM_TURN)
	{//上限に達したら
		m_TurnCnt = INT_ZERO;
		boss->ChangeState(new CWanderingState);
	}
}

//=============================================
//方向を動かして見渡す処理
//=============================================
void CBossConfusion::MoveRot(D3DXVECTOR3& rot, float Rot_Answer_y, CBossEnemy* boss)
{
	if (m_isRight)
	{
		rot.y += ROT_MOVE;

		if (Rot_Answer_y > LOOK_RANGE)
		{//範囲に到達したら逆回転
			m_isRight = false;
			++m_TurnCnt;
		}
	}
	if (!m_isRight)
	{
		rot.y -= ROT_MOVE;

		if (Rot_Answer_y < -LOOK_RANGE)
		{//範囲に到達したら逆回転
			m_isRight = true;
			++m_TurnCnt;
		}
	}
	boss->SetRot(rot);
}

//=============================================
//プレイヤーとレイが当たるのかチェック
//=============================================
CCharacter::RayHitInfo CBossConfusion::PerformRaycast_Player(D3DXVECTOR3 vector, CBossEnemy* boss)
{
	CCharacter::RayHitInfo Info; //ヒット情報を返す変数

	//初期化
	Info.distance = -1.0f; //絶対値で返るので当たらなかった時用に-を代入
	Info.hit = false; //当たっていない状態に

	for (int nCnt = 0; nCnt < CObject::MAX_OBJECT; nCnt++)
	{
		//オブジェクト取得
		CObject* pObj = CObject::Getobject(CPlayer::PLAYER_PRIORITY, nCnt);
		if (pObj != nullptr)
		{//ヌルポインタじゃなければ
		 //タイプ取得
			CObject::OBJECT_TYPE type = pObj->GetType();

			//敵との当たり判定
			if (type == CObject::OBJECT_TYPE::OBJECT_TYPE_PLAYER)
			{
				CPlayer* pPlayer = dynamic_cast<CPlayer*>(pObj);

				//レイを原点からの差分から飛ばす(yはエネミーから飛ばす際の高さ調整)
				D3DXVECTOR3 StartRay = {boss->GetPos().x - pPlayer->GetPos().x,boss->GetPos().y + CORRECTION_VALUE_Y,boss->GetPos().z - pPlayer->GetPos().z };
				for (int nParts = 0; nCnt < CPlayer::NUM_PARTS; nCnt++)
				{
					//レイを飛ばしプレイヤーと当たるかチェック
					D3DXIntersect(pPlayer->m_apModel[nCnt]->GetModelInfo(nCnt).pMesh, &StartRay, &vector, &Info.hit, NULL, NULL, NULL, &Info.distance, NULL, NULL);
					if (Info.hit)
					{
						return Info;
					}
				}
			}
		}
	}
	return Info;
}

//=============================================
//コンストラクタ
//=============================================
CBossGunAttack::CBossGunAttack()
{
}

//=============================================
//デストラクタ
//=============================================
CBossGunAttack::~CBossGunAttack()
{
}

//=============================================
//銃攻撃
//=============================================
void CBossGunAttack::GunAttack(CBullet::BULLET_ALLEGIANCE Allegiance, CBullet::BULLET_TYPE type, CCharacter* character)
{
	CBossEnemy::Motion_Type Motion;
	Motion = CBossEnemy::Motion_Type::MOTION_ATTACK;
	//モーション代入
	character->SetMotion(Motion);
	if (character->m_pGun->GetAmmo() > INT_ZERO)
	{
		if (character->m_pGunAttack != nullptr)
		{
			character->m_pGun->m_nRateCnt++;
			if (character->m_pGun->m_nRateCnt >= character->m_pGun->GetFireRate())
			{
				character->m_pGun->m_nRateCnt = INT_ZERO;
				D3DXVECTOR3 ShotPos = D3DXVECTOR3(character->m_apModel[14]->GetMtxWorld()._41 + sinf(character->GetRot().y + D3DX_PI) * 45.0f,
					character->m_apModel[14]->GetMtxWorld()._42 + 5.0f, character->m_apModel[14]->GetMtxWorld()._43 + cosf(character->GetRot().y + D3DX_PI) * 45.0f);

				D3DXVECTOR3 ShotMove = D3DXVECTOR3(sinf(character->GetRot().y + D3DX_PI) * character->m_pGun->GetBulletSpeed(),
					0.0f, cosf(character->GetRot().y + D3DX_PI) * character->m_pGun->GetBulletSpeed());

				character->m_pGun->m_nRateCnt = INT_ZERO;
				//弾発射
				character->m_pGun->m_pShot->Shot(ShotPos, ShotMove, character->m_pGun->m_Size, character->m_pGun->GetDamage(), Allegiance, type, character->m_pGun);					
			}
		}
	}
	else
	{
		character->m_pGun->m_pReload->Reload(character->m_pGun);
	}
}

//=============================================
//コンストラクタ
//=============================================
CBossTackle::CBossTackle():m_StayCnt(INT_ZERO), m_TackleCnt(INT_ZERO), m_effect_reduction(FLOAT_ZERO), m_isTackle(false)
{
}

//=============================================
//デストラクタ
//=============================================
CBossTackle::~CBossTackle()
{
}

//=============================================
//タックル攻撃
//=============================================
void CBossTackle::Tackle(CBossEnemy* boss)
{
	//リロード
	boss->m_pGun->Reload();

	if (!m_isTackle)
	{//タックルしてなかったら
		LookAtPlayer(boss);
		if (boss->m_pTackleCharge == nullptr)
		{
			//ダッシュエフェクト生成
			boss->m_pTackleCharge = CTackleCharge::Create({ {boss->GetPos().x,boss->GetPos().y + CORRECTION_VALUE_Y,boss->GetPos().z}}
			, boss->m_pTackleCharge->SIZE, COLOR_RED);

			//タックルのエフェクトの減産量計算
			m_effect_reduction = boss->m_pTackleCharge->SIZE.x / STAY_FLAME;
		}

		if (boss->m_pTackleCharge != nullptr)
		{
			//サイズ取得
			D3DXVECTOR3 size = boss->m_pTackleCharge->GetSize();

			size.x -= m_effect_reduction;
			size.y -= m_effect_reduction;

			boss->m_pTackleCharge->SetSize(size);
		}
		++m_StayCnt;
	}

	if (m_StayCnt > STAY_FLAME - CORRECTION_FLAME)
	{//カウントが既定カウントに到達したら
		//タックルSEを鳴らす
		CManager::GetInstance()->GetSound()->PlaySound(CSound::SOUND_LABEL_SE_TACKLE);

		if (boss->m_pTackleCharge != nullptr)
		{//エフェクトを破棄
			boss->m_pTackleCharge->Uninit();
			boss->m_pTackleCharge = nullptr;
		}
		m_isTackle = true;
		m_StayCnt = INT_ZERO;
	}

	if (m_isTackle)
	{
		++m_TackleCnt;

		//タックルの実行処理を呼ぶ
		boss->TackleAction();

		for (int nCnt = INT_ZERO; nCnt < boss->GetNumParts(); nCnt++)
		{
			if (m_TackleCnt > TACKLE_FLAME)
			{//何かに当たるか終了フレームに到達したら
				m_TackleCnt = INT_ZERO;
				m_StayCnt = INT_ZERO;
				m_isTackle = false;

				if (boss->m_pTackleEffect != nullptr)
				{//エフェクトがあったら
					//エフェクト破棄
					boss->m_pTackleEffect->Uninit();
					boss->m_pTackleEffect = nullptr;
				}
				boss->ChangeState(new CBossStanState);
			}

			if (boss->m_apModel[nCnt]->GetColisionBlockInfo().bColision_Z
				|| boss->m_apModel[nCnt]->GetColisionBlockInfo().bColision_X)
			{
				if (boss->m_apModel[nCnt]->GetColisionBlockInfo().pBlock != nullptr)
				{
					boss->m_apModel[nCnt]->GetColisionBlockInfo().pBlock->CreatePiece();
					boss->m_apModel[nCnt]->GetColisionBlockInfo().pBlock->Uninit();
					boss->m_apModel[nCnt]->GetColisionBlockInfo().pBlock = nullptr;

					//すべてのパーツを当たってない判定に
					boss->ColisionReset();
				}


				if (boss->m_pTackleEffect != nullptr)
				{//エフェクトがあったら
					//エフェクト破棄
					boss->m_pTackleEffect->Uninit();
					boss->m_pTackleEffect = nullptr;
				}

				//タックル情報初期化
				m_TackleCnt = INT_ZERO;
				m_StayCnt = INT_ZERO;
				m_isTackle = false;
				boss->ChangeState(new CBossStanState);
				break;
			}
		}
	}
}

//=============================================
//プレイヤーのほうを向かせる
//=============================================
void CBossTackle::LookAtPlayer(CCharacter* character)
{
	for (int nCnt = INT_ZERO; nCnt < CObject::MAX_OBJECT; nCnt++)
	{
		//オブジェクト取得
		CObject* pObj = CObject::Getobject(CPlayer::PLAYER_PRIORITY, nCnt);
		if (pObj != nullptr)
		{//ヌルポインタじゃなければ
			//タイプ取得
			CObject::OBJECT_TYPE type = pObj->GetType();

			//敵との当たり判定
			if (type == CObject::OBJECT_TYPE::OBJECT_TYPE_PLAYER)
			{
				CPlayer* pplayer = dynamic_cast<CPlayer*>(pObj);

				//プレイヤーとの距離算出
				D3DXVECTOR3 Distance = pplayer->GetPos() - character->GetPos();

				//プレイヤーに向ける角度を算出
				float fAngle = atan2f(Distance.x, Distance.z);

				//親クラスからrotを取得
				D3DXVECTOR3 rot = character->GetRot();

				rot.y = fAngle + D3DX_PI;

				character->SetRot(rot);
			}
		}
	}
}

//=============================================
//コンストラクタ
//=============================================
CBossStan::CBossStan()
{
}

//=============================================
//デストラクタ
//=============================================
CBossStan::~CBossStan()
{
}

//=============================================
//スタン処理
//=============================================
void CBossStan::Stan(CBossEnemy* boss)
{
}

//=============================================
//コンストラクタ
//=============================================
CBossSearch::CBossSearch()
{
}

//=============================================
//デストラクタ
//=============================================
CBossSearch::~CBossSearch()
{
}

//=============================================
//探索処理
//=============================================
void CBossSearch::Search(CBossEnemy* boss,D3DXVECTOR3 TargetPos)
{
	//対象の位置への方向情報
	D3DXVECTOR3 point = { TargetPos.x - boss->GetPos().x,FLOAT_ZERO,TargetPos.z - boss->GetPos().z };

	// 目的地との距離を計算
	float distance = sqrtf(point.x * point.x + point.z * point.z);

	// まだ目的地に到達していない場合のみ移動処理を行う
	if (distance > THRESHOLD)
	{
		//対象物との角度計算
		float angle = atan2f(point.x, point.z);

		D3DXVECTOR3 move = boss->GetMove();

		switch (boss->GetAxis())
		{
		case CBossEnemy::NONE:
			move.x += sinf(angle) * boss->GetSpeed();
			move.z += cosf(angle) * boss->GetSpeed();
			break;
		case CBossEnemy::X:
			boss->ChangeState(new CConfusionBossState);
			break;
		case CBossEnemy::Z:
			boss->ChangeState(new CConfusionBossState);
			break;
		default:
			break;
		}

		//親クラスからrotを取得
		D3DXVECTOR3 rot = boss->GetRot();
		rot.y = angle + D3DX_PI;
		//rotを代入
		boss->SetRot(rot);
		//移動量代入
		boss->SetMove(move);

		for (int nCnt = INT_ZERO; nCnt < CObject::MAX_OBJECT; nCnt++)
		{
			//オブジェクト取得
			CObject* pObj = CObject::Getobject(CPlayer::PLAYER_PRIORITY, nCnt);
			if (pObj != nullptr)
			{//ヌルポインタじゃなければ
				//タイプ取得
				CObject::OBJECT_TYPE type = pObj->GetType();

				//敵との当たり判定
				if (type == CObject::OBJECT_TYPE::OBJECT_TYPE_PLAYER)
				{
					CPlayer* player = dynamic_cast<CPlayer*>(pObj);
					//プレイヤーの位置への方向情報
					D3DXVECTOR3 Vector = player->GetPos() - boss->GetPos();

					if (boss->PerformRaycast_Player(Vector, boss).hit
						&& boss->PerformRaycast_Block(Vector, boss).distance > boss->PerformRaycast_Player(Vector, boss).distance)
					{
						boss->ChangeState(new CChaseState);
					}
				}
			}
		}

		if (boss->GetState() == CCharacter::CHARACTER_DAMAGE)
		{
			boss->ChangeState(new CChaseState);
		}
	}
	else
	{//到達していたら

		D3DXVECTOR3 move = VEC3_RESET_ZERO;
		boss->SetMove(move);

		boss->ChangeState(new CConfusionBossState);
	}
}

//外周のターゲットポイントのIdx
const int CBossRampage::OUTER_CIRCUMFERENCE[NUM_TARGETPOINT]
{
	0,
	2,
	8,
	6,
};

//=============================================
//コンストラクタ
//=============================================
CBossRampage::CBossRampage():m_MoveIdx(INT_ZERO), m_LapCnt(INT_ZERO)
{
}

//=============================================
//デストラクタ
//=============================================
CBossRampage::~CBossRampage()
{
}

//=============================================
//暴走状態
//=============================================
void CBossRampage::Rampage(CBossEnemy* boss)
{
	//移動先の情報取得(引数が移動先のポイントの配列番号)
	CMovePoint* pMovePoint = CWave_Boss::GetMovePoint(OUTER_CIRCUMFERENCE[m_MoveIdx]);

	//対象の位置への方向情報
	D3DXVECTOR3 point = { pMovePoint->GetPos().x - boss->GetPos().x ,FLOAT_ZERO ,pMovePoint->GetPos().z - boss->GetPos().z };

	// 目的地との距離を計算
	float distance = sqrtf(point.x * point.x + point.z * point.z);

	//プレイヤーに向ける角度を算出
	float fAngle = atan2f(point.x, point.z);

	//親クラスからrotを取得
	D3DXVECTOR3 rot = boss->GetRot();

	rot.y = fAngle + D3DX_PI;

	boss->SetRot(rot);

	for (int nCnt = INT_ZERO; nCnt < boss->GetNumParts(); nCnt++)
	{
		if (boss->m_apModel[nCnt]->GetColisionBlockInfo().bColision_Z
			|| boss->m_apModel[nCnt]->GetColisionBlockInfo().bColision_X)
		{
			if (boss->m_apModel[nCnt]->GetColisionBlockInfo().pBlock != nullptr)
			{
				boss->m_apModel[nCnt]->GetColisionBlockInfo().pBlock->CreatePiece();
				boss->m_apModel[nCnt]->GetColisionBlockInfo().pBlock->Uninit();
				boss->m_apModel[nCnt]->GetColisionBlockInfo().pBlock = nullptr;
				//すべてのパーツを当たってない判定に
				boss->ColisionReset();
			}
		}
	}

	// まだ目的地に到達していない場合のみ移動処理を行う
	if (distance > THRESHOLD)
	{
		//タックルの実行処理を呼ぶ
		boss->TackleAction();
	}
	else
	{
		if (boss->m_pTackleEffect != nullptr)
		{//エフェクトがあったら
			//エフェクト破棄
			boss->m_pTackleEffect->Uninit();
			boss->m_pTackleEffect = nullptr;
		}

		++m_MoveIdx; //位置カウントアップ
		if (m_MoveIdx > NUM_TARGETPOINT)
		{
			boss->SetMove(VEC3_RESET_ZERO);

			m_LapCnt++; //周回数カウントアップ
			m_MoveIdx = INT_ZERO; //初期位置に
		}
		if (m_LapCnt >= NUM_RAPS)
		{
			//メンバ変数リセット
			m_LapCnt = INT_ZERO;
			m_MoveIdx = INT_ZERO;
			boss->SetMotion(CBossEnemy::MOTION_NEUTRAL);
			boss->SetMove(VEC3_RESET_ZERO);
			boss->ChangeState(new CTackleState);
		}
	}
}
