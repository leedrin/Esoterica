#include "Animation_ToolsGraphNode_BoneMasks.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_BoneMasks.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    BoneMaskToolsNode::BoneMaskToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Bone Mask", GraphValueType::BoneMask, true );
    }

    int16_t BoneMaskToolsNode::Compile( GraphCompilationContext& context ) const
    {
        BoneMaskNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<BoneMaskNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( !m_maskID.IsValid() )
            {
                context.LogError( this, "Invalid Bone Mask ID" );
                return InvalidIndex;
            }

            pSettings->m_boneMaskID = m_maskID;
        }

        return pSettings->m_nodeIdx;
    }

    void BoneMaskToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        DrawInternalSeparator( ctx );

        ImGui::Text( "Mask ID: %s", m_maskID.IsValid() ? m_maskID.c_str() : "NONE");
    }

    void BoneMaskToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        Variation const* pSelectedVariation = pGraphNodeContext->m_pVariationHierarchy->GetVariation( pGraphNodeContext->m_selectedVariationID );

        if ( pSelectedVariation->m_skeleton.GetResourceID().IsValid() )
        {
            pUserContext->RequestOpenResource( pSelectedVariation->m_skeleton.GetResourceID() );
        }
    }

    //-------------------------------------------------------------------------

    FixedWeightBoneMaskToolsNode::FixedWeightBoneMaskToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Bone Mask", GraphValueType::BoneMask, true );
    }

    int16_t FixedWeightBoneMaskToolsNode::Compile( GraphCompilationContext& context ) const
    {
        FixedWeightBoneMaskNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<FixedWeightBoneMaskNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            pSettings->m_boneWeight = m_boneWeight;
        }

        return pSettings->m_nodeIdx;
    }

    void FixedWeightBoneMaskToolsNode::DrawInfoText( VisualGraph::DrawContext const& ctx )
    {
        DrawInternalSeparator( ctx );

        ImGui::Text( "Mask Weight: %.2f", Math::Clamp( m_boneWeight, 0.0f, 1.0f ) );
    }

    //-------------------------------------------------------------------------

    BoneMaskBlendToolsNode::BoneMaskBlendToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::BoneMask, true );
        CreateInputPin( "Blend Weight", GraphValueType::Float );
        CreateInputPin( "Source", GraphValueType::BoneMask );
        CreateInputPin( "Target", GraphValueType::BoneMask );
    }

    int16_t BoneMaskBlendToolsNode::Compile( GraphCompilationContext& context ) const
    {
        BoneMaskBlendNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<BoneMaskBlendNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_blendWeightValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_sourceMaskNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_targetMaskNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }
        }

        return pSettings->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    BoneMaskSelectorToolsNode::BoneMaskSelectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::BoneMask, true );
        CreateInputPin( "Parameter", GraphValueType::ID );
        CreateInputPin( "Default Mask", GraphValueType::BoneMask );
        CreateInputPin( "Mask 0", GraphValueType::BoneMask );

        m_parameterValues.emplace_back();
    }

    int16_t BoneMaskSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        BoneMaskSelectorNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<BoneMaskSelectorNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                int16_t const compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_parameterValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on selector node!" );
                return InvalidIndex;
            }

            // Default Mask
            //-------------------------------------------------------------------------

            auto pDefaultMaskNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pDefaultMaskNode != nullptr )
            {
                int16_t const compiledNodeIdx = pDefaultMaskNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_defaultMaskNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            // Dynamic Options
            //-------------------------------------------------------------------------

            int32_t const numInputs = GetNumInputPins();
            int32_t const numDynamicOptions = numInputs - 2;

            for ( auto i = 2; i < numInputs; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_maskNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected option pin on selector node!" );
                    return InvalidIndex;
                }
            }

            // Validate parameter values
            //-------------------------------------------------------------------------

            if ( m_parameterValues.size() < numDynamicOptions )
            {
                context.LogError( "Less parameters set than we have options! Please add missing parameter values" );
                return InvalidIndex;
            }

            if ( m_parameterValues.size() > numDynamicOptions )
            {
                context.LogWarning( "More parameters set than we have options, extra parameters will be ignored!" );
            }

            for ( auto parameter : m_parameterValues )
            {
                if ( !parameter.IsValid() )
                {
                    context.LogError( "Invalid parameter value set for bone mask selector!" );
                    return InvalidIndex;
                }
            }

            // Set parameter values
            //-------------------------------------------------------------------------

            pSettings->m_parameterValues.clear();
            pSettings->m_parameterValues.insert( pSettings->m_parameterValues.begin(), m_parameterValues.begin(), m_parameterValues.begin() + numDynamicOptions );

            //-------------------------------------------------------------------------

            pSettings->m_switchDynamically = m_switchDynamically;
            pSettings->m_blendTime = m_blendTime;
        }

        return pSettings->m_nodeIdx;
    }

    TInlineString<100> BoneMaskSelectorToolsNode::GetNewDynamicInputPinName() const
    {
        int32_t const numInputPins = GetNumInputPins();
        TInlineString<100> pinName;
        pinName.sprintf( "Mask %d", numInputPins - 3 );
        return pinName;
    }

    void BoneMaskSelectorToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        m_parameterValues.emplace_back();
    }

    void BoneMaskSelectorToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        //-------------------------------------------------------------------------

        int32_t const numInputPins = GetNumInputPins();
        int32_t const numOptions = ( numInputPins - 2 );
        if ( m_parameterValues.size() == numOptions )
        {
            m_parameterValues.erase( m_parameterValues.begin() + pinToBeRemovedIdx - 2 );
        }

        // Rename Pins
        //-------------------------------------------------------------------------

        int32_t newPinIdx = 1;
        for ( auto i = 2; i < numInputPins; i++ )
        {
            if ( i == pinToBeRemovedIdx )
            {
                continue;
            }

            TInlineString<100> newPinName;
            newPinName.sprintf( "Mask %d", newPinIdx );
            GetInputPin( i )->m_name = newPinName;
            newPinIdx++;
        }
    }

    void BoneMaskSelectorToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        for ( auto const& ID : m_parameterValues )
        {
            outIDs.emplace_back( ID );
        }
    }

    void BoneMaskSelectorToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = false;
        for ( auto const& ID : m_parameterValues )
        {
            if ( ID == oldID )
            {
                foundMatch = true;
                break;
            }
        }

        if ( foundMatch )
        {
            VisualGraph::ScopedNodeModification snm( this );
            for ( auto& ID : m_parameterValues )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }
        }
    }
}